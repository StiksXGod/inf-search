import sys
import time
import yaml
import requests
import hashlib
import argparse
import re
from urllib.parse import urlparse, urljoin, urlunparse
from datetime import datetime, timezone
from pymongo import MongoClient, errors
from bs4 import BeautifulSoup

class SearchBot:
    def __init__(self, config_path):
        self.load_config(config_path)
        self.setup_db()
        self.session = requests.Session()
        self.session.headers.update({'User-Agent': self.config['logic']['user_agent']})

    def load_config(self, path):
        with open(path, 'r') as f:
            self.config = yaml.safe_load(f)

    def setup_db(self):
        db_conf = self.config['db']
        client = MongoClient(db_conf['host'], db_conf['port'])
        self.db = client[db_conf['name']]
        self.documents = self.db[db_conf['collection']]
        self.queue = self.db['queue']
        
        self.documents.create_index("url", unique=True)
        self.queue.create_index("url", unique=True)

    def normalize_url(self, url):
        parsed = urlparse(url)
        return urlunparse((parsed.scheme, parsed.netloc, parsed.path, parsed.params, parsed.query, ''))

    def get_source(self, url):
        domain = urlparse(url).netloc
        if 'ria.ru' in domain:
            return 'ria'
        elif 'rbc.ru' in domain:
            return 'rbc'
        return 'other'

    def is_article(self, url):
        if 'ria.ru' in url:
            return bool(re.search(r'ria\.ru/.*-\d+\.html', url))
        elif 'rbc.ru' in url:
            return bool(re.search(r'rbc\.ru/.*[0-9a-f]{24}', url))
        return False

    def add_to_queue(self, url, source, depth=0):
        normalized_url = self.normalize_url(url)
        
        if self.queue.find_one({"url": normalized_url}):
            return

        doc = self.documents.find_one({"url": normalized_url})
        if doc:
            if self.needs_recrawl(doc):
                priority = 10 if self.is_article(normalized_url) else 1
                self.queue.insert_one({
                    "url": normalized_url,
                    "source": source,
                    "depth": depth,
                    "status": "pending",
                    "priority": priority,
                    "next_crawl": 0
                })
            return

        priority = 10 if self.is_article(normalized_url) else 1
        self.queue.insert_one({
            "url": normalized_url,
            "source": source,
            "depth": depth,
            "status": "pending",
            "priority": priority,
            "next_crawl": 0
        })

    def needs_recrawl(self, doc):
        recrawl_interval = self.config['logic'].get('recrawl_interval', 86400)
        last_crawled = doc.get('crawled_at', 0)
        return (time.time() - last_crawled) > recrawl_interval

    def fetch(self, url):
        try:
            response = self.session.get(url, timeout=10, stream=True)
            
            content_type = response.headers.get('Content-Type', '')
            if 'text/html' not in content_type:
                print(f"Skipping non-html content: {url} ({content_type})")
                response.close()
                return None

            response.raise_for_status()
            return response.text
        except Exception as e:
            print(f"Error fetching {url}: {e}")
            return None

    def refill_queue_with_expired(self):
        print("Checking for expired documents...")
        recrawl_interval = self.config['logic'].get('recrawl_interval', 86400)
        cutoff_time = int(time.time()) - recrawl_interval
        
        cursor = self.documents.find({"crawled_at": {"$lt": cutoff_time}})
        count = 0
        for doc in cursor:
            if not self.queue.find_one({"url": doc['url']}):
                self.queue.insert_one({
                    "url": doc['url'],
                    "source": doc['source'],
                    "depth": 0, 
                    "status": "pending",
                    "next_crawl": 0
                })
                count += 1
        
        if count > 0:
            print(f"Added {count} expired documents to queue.")
        else:
            print("No expired documents found.")

    def process_url(self, item):
        url = item['url']
        source = item['source']
        depth = item.get('depth', 0)
        
        print(f"Processing: {url}")
        
        html = self.fetch(url)
        if not html:
            self.queue.delete_one({"_id": item["_id"]}) 
            return

        current_hash = hashlib.md5(html.encode('utf-8')).hexdigest()
        timestamp = int(time.time())
        
        is_article = self.is_article(url)
        doc_type = 'article' if is_article else 'navigation'

        existing_doc = self.documents.find_one({"url": url})
        
        if existing_doc:
            update_data = {"crawled_at": timestamp}
            
            if existing_doc.get('hash') != current_hash:
                print(f"Updating {doc_type}: {url}")
                update_data.update({
                    "html": html,
                    "hash": current_hash,
                    "type": doc_type
                })
            elif existing_doc.get('type') != doc_type:
                print(f"Updating type to {doc_type}: {url}")
                update_data['type'] = doc_type
            else:
                pass

            self.documents.update_one(
                {"_id": existing_doc["_id"]},
                {"$set": update_data}
            )
        else:
            if doc_type == 'article':
                print(f"[+] New ARTICLE: {url}")
            else:
                print(f"[ ] New navigation: {url}")

            self.documents.insert_one({
                "url": url,
                "html": html,
                "source": source,
                "crawled_at": timestamp,
                "hash": current_hash,
                "type": doc_type
            })

        max_depth = self.config['logic'].get('max_depth', 2)
        if depth < max_depth:
            soup = BeautifulSoup(html, 'html.parser')
            for link in soup.find_all('a', href=True):
                href = link['href']
                full_url = urljoin(url, href)
                
                new_source = self.get_source(full_url)
                if new_source in ['ria', 'rbc']:
                    self.add_to_queue(full_url, new_source, depth + 1)

        self.queue.delete_one({"_id": item["_id"]})

    def run(self):
        if self.queue.count_documents({}) == 0 and self.documents.count_documents({}) == 0:
            for seed in self.config['seed_urls']:
                self.add_to_queue(seed['url'], seed['source'])

        print("Starting crawler...")
        max_docs = self.config['logic'].get('max_documents', 30000)
        
        processed_count = 0
        while True:
            if processed_count % 10 == 0:
                current_docs = self.documents.count_documents({"type": "article"})
                print(f"Progress: {current_docs}/{max_docs} articles collected.")
                if current_docs >= max_docs:
                    print(f"Reached limit of {max_docs} articles. Stopping.")
                    break

            item = self.queue.find_one(
                {"status": "pending"},
                sort=[("priority", -1), ("_id", 1)]
            )
            
            if not item:
                print("Queue empty. Checking for expired docs...")
                self.refill_queue_with_expired()
                
                item = self.queue.find_one({"status": "pending"})
                if not item:
                    print("Queue still empty. Sleeping...")
                    time.sleep(5)
                    continue

            self.process_url(item)
            processed_count += 1
            
            delay = self.config['logic'].get('delay', 1.0)
            time.sleep(delay)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Search Bot')
    parser.add_argument('config', help='Path to config.yaml')
    args = parser.parse_args()

    bot = SearchBot(args.config)
    try:
        bot.run()
    except KeyboardInterrupt:
        print("\nStopping bot...")
        sys.exit(0)
