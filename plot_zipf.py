import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

def plot_zipf(csv_file):
    try:
        df = pd.read_csv(csv_file)
    except FileNotFoundError:
        print(f"Error: File {csv_file} not found. Run tokenizer first.")
        return

    plt.figure(figsize=(12, 8))
    
    plt.loglog(df['rank'], df['frequency'], marker='.', linestyle='none', markersize=2, label='Experimental Data')

    if not df.empty:
        c = df.iloc[0]['frequency']
        theoretical_freq = [c / r for r in df['rank']]
        plt.loglog(df['rank'], theoretical_freq, 'r--', label='Zipf\'s Law (Theoretical, alpha=1)')


    C = df.iloc[0]['frequency']
    zipf_theoretical = [C / r for r in df['rank']]
    
    plt.loglog(df['rank'], zipf_theoretical, 'r--', linewidth=2, label="Zipf's Law (Theoretical)")

    plt.title("Zipf's Law: Rank vs Frequency (Log-Log Scale)")
    plt.xlabel("Rank")
    plt.ylabel("Frequency")
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.2)
    
    output_file = 'zipf_plot.png'
    plt.savefig(output_file)
    print(f"Plot saved to {output_file}")

if __name__ == "__main__":
    csv_path = sys.argv[1] if len(sys.argv) > 1 else 'frequency_data.csv'
    plot_zipf(csv_path)
