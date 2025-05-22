# find_jsonl.py
import glob

for path in glob.glob("**/*.jsonl", recursive=True):
    print(path)
