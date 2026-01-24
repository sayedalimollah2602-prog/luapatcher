"""
Generate games_index.json from all Lua files in the games directory.
This script is run by the GitHub Actions workflow on each push.
"""

import os
import json


def generate_index():
    """Scan games directory and generate JSON index of all app IDs."""
    games_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')
    
    app_ids = []
    
    if os.path.exists(games_dir):
        for filename in os.listdir(games_dir):
            if filename.endswith('.lua'):
                # Extract app ID from filename (e.g., "730.lua" -> "730")
                app_id = filename[:-4]
                app_ids.append(app_id)
    
    # Sort for consistent output
    app_ids.sort(key=lambda x: int(x) if x.isdigit() else 0)
    
    index_data = {
        'app_ids': app_ids,
        'count': len(app_ids)
    }
    
    # Write to root of webserver folder
    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games_index.json')
    with open(output_path, 'w') as f:
        json.dump(index_data, f)
    
    print(f"Generated games_index.json with {len(app_ids)} app IDs")
    return output_path


if __name__ == '__main__':
    generate_index()
