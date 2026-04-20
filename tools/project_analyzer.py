import os
import re
import json
from pathlib import Path
from datetime import datetime
from collections import Counter

# You will need to install gitpython: pip install gitpython
try:
    import git
except ImportError:
    print("Error: 'gitpython' library not found. Run 'pip install gitpython' to use this tool.")
    exit(1)

class CaravanAnalyzer:
    def __init__(self, repo_path="."):
        try:
            self.repo = git.Repo(repo_path)
            self.root = Path(self.repo.working_tree_dir)
        except git.InvalidGitRepositoryError:
            print(f"Error: {repo_path} is not a valid git repository.")
            exit(1)

        # File categories for a graphics project
        self.extensions = {
            '.cpp': 'Source',
            '.hpp': 'Header',
            '.h': 'Header',
            '.vert': 'Shader',
            '.frag': 'Shader',
            '.geom': 'Shader',
            '.glsl': 'Shader',
            '.py': 'Tooling',
            '.json': 'Data'
        }

    def get_tracked_files(self):
        """Returns a list of files currently tracked by Git."""
        return self.repo.git.ls_files().splitlines()

    def analyze_file(self, rel_path):
        """Perform static and git analysis on a single file."""
        abs_path = self.root / rel_path
        ext = abs_path.suffix
        category = self.extensions.get(ext, 'Other')

        # Git Stats
        commits = list(self.repo.iter_commits(paths=rel_path))
        churn = len(commits)
        last_mod = commits[0].committed_datetime.isoformat() if commits else "Never"

        # Content Stats
        line_count = 0
        x_macro_count = 0
        if abs_path.is_file():
            try:
                with open(abs_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                    line_count = len(lines)
                    
                    # Logic to detect your OpenGL X-Macros: X(ReturnType, Name, ...)
                    content = "".join(lines)
                    x_macro_count = len(re.findall(r'X\s*\(\s*\w+\s*,\s*\w+', content))
            except Exception as e:
                pass # Binary or unreadable file

        return {
            "file": rel_path,
            "category": category,
            "lines": line_count,
            "churn": churn,
            "last_modified": last_mod,
            "x_macros": x_macro_count
        }

    def run(self):
        print(f"--- Analyzing Project: {self.root.name} ---")
        tracked_files = self.get_tracked_files()
        
        results = []
        for f in tracked_files:
            results.append(self.analyze_file(f))

        # Aggregation
        total_lines = sum(r['lines'] for r in results)
        total_macros = sum(r['x_macros'] for r in results)
        cat_counts = Counter(r['category'] for r in results)
        
        # Identify "Hotspots" (High churn and high line count)
        # Formula: churn * (lines / 100)
        hotspots = sorted(results, key=lambda x: x['churn'] * (x['lines']/100.0), reverse=True)[:5]

        # Output Summary
        print(f"Total Tracked Files: {len(results)}")
        print(f"Total Lines of Code: {total_lines}")
        print(f"Total OpenGL X-Macros: {total_macros}")
        print("\nFile Breakdown:")
        for cat, count in cat_counts.items():
            print(f"  {cat: <10}: {count}")

        print("\nTop Hotspots (Files needing refactoring):")
        for h in hotspots:
            if h['lines'] > 0:
                print(f"  {h['file']: <30} | Churn: {h['churn']: >3} | Lines: {h['lines']: >5}")

        # Export to JSON for your C++ engine to potentially read
        with open(self.root / "project_stats.json", "w") as jf:
            json.dump({
                "timestamp": datetime.now().isoformat(),
                "summary": {
                    "files": len(results),
                    "lines": total_lines,
                    "macros": total_macros
                },
                "details": results
            }, jf, indent=4)
        print(f"\nReport exported to project_stats.json")

def main():
    # You can change "." to any git repo path
    analyzer = CaravanAnalyzer(".")
    analyzer.run()

if __name__ == "__main__":
    main()