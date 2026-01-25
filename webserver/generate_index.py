"""
Generate games_index.json from all Lua files in the games directory.
This script is run by the GitHub Actions workflow on each push.
It fetches game names from the Steam API to create a rich index.
"""

import os
import json
import requests
import time

STEAM_APP_LIST_URL = "http://api.steampowered.com/ISteamApps/GetAppList/v2"

def get_steam_app_map():
    """Fetch all steam apps and return a dict of {app_id: name}."""
    try:
        print("Fetching Steam App List...")
        response = requests.get(STEAM_APP_LIST_URL, timeout=30)
        response.raise_for_status()
        data = response.json()
        
        app_map = {}
        for app in data['applist']['apps']:
            app_map[str(app['appid'])] = app['name']
            
        print(f"Fetched {len(app_map)} apps from Steam.")
        return app_map
    except Exception as e:
        print(f"Error fetching Steam App List: {e}")
        return {}

def generate_index():
    """Scan games directory and generate JSON index of all supported games with names."""
    games_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')
    
    # Get mapping of AppID -> Name
    app_map = get_steam_app_map()
    
    games_list = []
    
    if os.path.exists(games_dir):
        for filename in os.listdir(games_dir):
            if filename.endswith('.lua'):
                # Extract app ID from filename (e.g., "730.lua" -> "730")
                app_id = filename[:-4]
                
                # Get name from map, or fallback to generic
                name = app_map.get(app_id, f"Unknown Game ({app_id})")
                
                games_list.append({
                    "id": app_id,
                    "name": name
                })
    
    # Sort by name for easier reading, though client might resort
    games_list.sort(key=lambda x: x['name'])
    
    index_data = {
        'games': games_list,
        'count': len(games_list),
        'last_updated': int(time.time())
    }
    
    # Write to root of webserver folder
    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games_index.json')
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(index_data, f, ensure_ascii=False, indent=2)
    
    print(f"Generated games_index.json with {len(games_list)} supported games.")
    return output_path


if __name__ == '__main__':
    generate_index()
