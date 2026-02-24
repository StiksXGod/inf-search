import sys
import os
import re
from pymongo import MongoClient
from bs4 import BeautifulSoup
import yaml

CONFIG_PATH = os.path.join(os.path.dirname(__file__), 'config.yaml')

def load_config(path):
    with open(path, 'r') as f:
        return yaml.safe_load(f)

def clean_text(html):
    soup = BeautifulSoup(html, 'html.parser')
    
    for script in soup(["script", "style", "header", "footer", "nav"]):
        script.extract()
        
    text = soup.get_text(separator=' ')
    
    lines = (line.strip() for line in text.splitlines())
    chunks = (phrase.strip() for line in lines for phrase in line.split("  "))
    text = ' '.join(chunk for chunk in chunks if chunk)
    
    return text

def export_data():
    config = load_config(CONFIG_PATH)
    db_conf = config['db']
    
    client = MongoClient(db_conf['host'], db_conf['port'])
    db = client[db_conf['name']]
    collection = db[db_conf['collection']]
    
    output_file = os.path.join(os.path.dirname(__file__), 'data/corpus.txt')
    urls_file = os.path.join(os.path.dirname(__file__), 'data/urls.txt')
    
    
    print(f"Exporting articles to {output_file} and {urls_file}...")
    
    count = 0
    with open(output_file, 'w', encoding='utf-8') as f_text, \
         open(urls_file, 'w', encoding='utf-8') as f_urls:
        cursor = collection.find({"type": "article"})
        total = collection.count_documents({"type": "article"})
        
        for doc in cursor:
            if 'html' in doc and 'url' in doc:
                text = clean_text(doc['html'])

                text = text.replace('\n', ' ').replace('\r', ' ')
                
                f_text.write(text + "\n")
                f_urls.write(doc['url'] + "\n")
                
                count += 1
                if count % 100 == 0:
                    print(f"Processed {count}/{total}", end='\r')
                    
    print(f"\nDone! Exported {count} documents.")

if __name__ == "__main__":
    export_data()
