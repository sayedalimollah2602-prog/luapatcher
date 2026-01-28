"""
Generate games_index.json from all Lua files in the games directory.
This script is run by the GitHub Actions workflow on each push.
It fetches game names from the Steam API to create a rich index.

Usage:
    python generate_index.py                    # Without API key (may hit rate limits)
    STEAM_API_KEY=xxx python generate_index.py  # With API key (recommended)
"""

import os
import json
import requests
import time
import signal
import sys

# Get Steam API key from environment variable (optional but recommended)
STEAM_API_KEY = os.environ.get('STEAM_API_KEY', '')

# Maximum runtime in seconds (5 hours)
MAX_RUNTIME_SECONDS = 5 * 60 * 60

# Global variables for graceful shutdown on CTRL+C
_progress_file = None
_extracted_names = {}
_completed_ids = []
_current_app_map = {}  # Base map before fetching new ones

def save_games_index(app_map):
    """Generate and write the games_index.json file."""
    games_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')
    games_list = []
    
    if os.path.exists(games_dir):
        for filename in os.listdir(games_dir):
            if filename.endswith('.lua'):
                app_id = filename[:-4]
                name = app_map.get(app_id, f"Unknown Game ({app_id})")
                
                games_list.append({
                    "id": app_id,
                    "name": name
                })
    
    # Sort by name
    games_list.sort(key=lambda x: x['name'])
    
    index_data = {
        'games': games_list,
        'count': len(games_list),
        'last_updated': int(time.time())
    }
    
    # Write to root of webserver folder
    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games_index.json')
    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(index_data, f, ensure_ascii=False, indent=2)
        print(f"Generated games_index.json with {len(games_list)} supported games.")
        return output_path
    except Exception as e:
        print(f"Error writing index file: {e}")
        return None

def signal_handler(signum, frame):
    """Handle CTRL+C by saving progress and exiting gracefully."""
    print(f"\n\n=== INTERRUPTED (CTRL+C) ===")
    
    # 1. Save fetch progress (for resuming)
    if _progress_file and (_extracted_names or _completed_ids):
        print("Saving fetch progress...")
        try:
            with open(_progress_file, 'w') as f:
                json.dump({
                    'names': _extracted_names,
                    'completed_ids': _completed_ids
                }, f)
            print(f"Saved {len(_extracted_names)} names to {_progress_file}")
        except Exception as e:
            print(f"Error saving progress: {e}")

    # 2. Save games_index.json with what we have so far
    print("Updating games_index.json with collected data...")
    try:
        # Merge base map with extracted names
        final_map = _current_app_map.copy()
        final_map.update(_extracted_names)
        save_games_index(final_map)
    except Exception as e:
        print(f"Error updating index: {e}")

    print("Exiting...")
    sys.exit(0)

# Register signal handler
signal.signal(signal.SIGINT, signal_handler)

def load_existing_games_index():
    """Load existing games_index.json to avoid re-fetching known games."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    index_path = os.path.join(script_dir, 'games_index.json')
    
    if os.path.exists(index_path):
        try:
            with open(index_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
                # Build a map of id -> name, excluding "Unknown Game" entries
                existing = {}
                for game in data.get('games', []):
                    name = game.get('name', '')
                    if name and not name.startswith('Unknown Game'):
                        existing[game['id']] = name
                print(f"Loaded {len(existing)} existing game names from games_index.json")
                return existing
        except Exception as e:
            print(f"Could not load existing index: {e}")
    return {}

def get_steam_app_map():
    """Fetch all steam apps and return a dict of {app_id: name}."""
    print("Fetching Steam App List...")
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
    }
    
    # Build URL list - prioritize API key URL if available
    urls = []
    if STEAM_API_KEY:
        urls.append(f"https://api.steampowered.com/ISteamApps/GetAppList/v2/?key={STEAM_API_KEY}")
        print("Using Steam API key for authentication")
    urls.extend([
        "https://api.steampowered.com/ISteamApps/GetAppList/v2/",
        "https://raw.githubusercontent.com/SteamDatabase/SteamAppList/main/games.json",
        "https://raw.githubusercontent.com/leinstay/steamdb/main/steamdb.json"
    ])

    for url in urls:
        try:
            # Don't print full URL if it contains API key
            display_url = url.split('?')[0] + ('?key=***' if 'key=' in url else '')
            print(f"Trying {display_url}...")
            response = requests.get(url, headers=headers, timeout=30)
            if response.status_code == 200:
                data = response.json()
                app_map = {}
                
                # Handle different JSON structures
                apps = []
                if 'applist' in data and 'apps' in data['applist']:
                    apps = data['applist']['apps']
                elif 'applist' in data and 'apps' in data:
                     apps = data['applist']['apps']
                elif 'apps' in data:
                    apps = data['apps']
                elif isinstance(data, list):
                    apps = data
                
                if not apps and 'applist' in data:
                     apps = data['applist'].get('apps', [])

                for app in apps:
                    app_map[str(app['appid'])] = app['name']
                
                print(f"Fetched {len(app_map)} apps from Steam API.")
                return app_map
            else:
                print(f"Failed with status {response.status_code}")
                # print(f"Response: {response.text[:300]}") # Less verbose if failing
        except Exception as e:
            print(f"Error fetching from {url}: {e}")
            
    print("All Steam App List URLs failed.")
    return {}

def fetch_names_from_store_api(app_ids):
    """Fetch specific app names from the Store API if bulk list fails."""
    global _progress_file, _extracted_names, _completed_ids
    
    if not app_ids:
        return {}
    
    # Progress file for resumability
    script_dir = os.path.dirname(os.path.abspath(__file__))
    _progress_file = os.path.join(script_dir, 'fetch_progress.json')
    
    # Load existing progress if available
    _extracted_names = {}
    if os.path.exists(_progress_file):
        try:
            with open(_progress_file, 'r') as f:
                progress_data = json.load(f)
                _extracted_names = progress_data.get('names', {})
                completed_ids_set = set(progress_data.get('completed_ids', []))
                # Filter out already completed IDs
                remaining_ids = [aid for aid in app_ids if aid not in completed_ids_set]
                print(f"Resuming from progress file. Already fetched: {len(completed_ids_set)} names.")
                app_ids = remaining_ids
        except Exception as e:
            print(f"Could not load progress file: {e}")
    
    if not app_ids:
        print("All apps already fetched from previous run.")
        return _extracted_names
        
    print(f"Attempting to fetch {len(app_ids)} missing game names from Store API (one by one)...")
    print(f"Estimated time: ~{len(app_ids) * 1.5 / 60:.1f} minutes (with rate limiting)")
    print(f"Maximum runtime: 5 hours (will auto-stop and save progress)")
    print(f"Press CTRL+C anytime to save progress and exit.\n")
    
    base_url = "https://store.steampowered.com/api/appdetails"
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
    }
    
    _completed_ids = list(_extracted_names.keys())
    
    total = len(app_ids)
    save_interval = 100  # Save progress every 100 games
    start_time = time.time()  # Track start time for timeout
    
    for i, app_id in enumerate(app_ids):
        # Progress indication
        progress_pct = (i + 1) / total * 100
        print(f"[{progress_pct:.1f}%] Fetching {i + 1}/{total}: AppID {app_id}...", end=" ")
        
        # Define sources to try in order
        # Source 1: Steam Store API (Official, reliable but rate limited)
        # Source 2: SteamSpy API (Unofficial, good fallback)
        sources = [
            ("Store", f"https://store.steampowered.com/api/appdetails?appids={app_id}"),
            ("SteamSpy", f"https://steamspy.com/api.php?request=appdetails&appid={app_id}")
        ]
        
        fetched_name = None
        
        for source_name, url in sources:
            try:
                # Different headers/params might be needed per source
                # Common headers
                req_headers = {
                    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
                }
                
                # Fetch
                response = requests.get(url, headers=req_headers, timeout=10)
                
                if response.status_code == 200:
                    data = response.json()
                    
                    # Parse based on source
                    if source_name == "Store":
                        if data and str(app_id) in data:
                            details = data[str(app_id)]
                            if details.get("success") and "data" in details:
                                fetched_name = details["data"]["name"]
                                
                    elif source_name == "SteamSpy":
                        # SteamSpy format: {"appid": 123, "name": "Game Name", ...}
                        if data and "name" in data:
                            fetched_name = data["name"]
                            
                    if fetched_name:
                        print(f"-> Found via {source_name}: {fetched_name}")
                        _extracted_names[str(app_id)] = fetched_name
                        break # Stop trying sources
                
                elif response.status_code == 429:
                     print(f"-> {source_name} Rate Limit! Waiting 10s...", end=" ")
                     time.sleep(10)
                else:
                    # Fail silently for this source, try next
                    pass

            except Exception as e:
                # Error with this source, try next
                pass
        
        if not fetched_name:
             print("-> Failed (all sources)")
        
        _completed_ids.append(app_id)
        
        # Save progress periodically
        if (i + 1) % save_interval == 0:
            elapsed = time.time() - start_time
            elapsed_hours = elapsed / 3600
            print(f"\n--- Saving progress ({len(_extracted_names)} names fetched, {elapsed_hours:.1f}h elapsed) ---\n")
            with open(_progress_file, 'w') as f:
                json.dump({
                    'names': _extracted_names,
                    'completed_ids': _completed_ids
                }, f)
        
        # Check for timeout (5 hours)
        elapsed = time.time() - start_time
        if elapsed >= MAX_RUNTIME_SECONDS:
            print(f"\n\n=== 5 HOUR TIMEOUT REACHED ===")
            print(f"Saving progress and exiting...")
            
            # 1. Save fetch progress
            with open(_progress_file, 'w') as f:
                json.dump({
                    'names': _extracted_names,
                    'completed_ids': _completed_ids
                }, f)
                
            # 2. Save games_index.json
            # Merge base map with extracted names
            final_map = _current_app_map.copy()
            final_map.update(_extracted_names)
            save_games_index(final_map)

            print(f"Saved {len(_extracted_names)} names. Run script again to continue.")
            return _extracted_names
            
        time.sleep(1)  # Rate limit protection (1 second between requests)
    
    # Final save
    with open(_progress_file, 'w') as f:
        json.dump({
            'names': _extracted_names,
            'completed_ids': _completed_ids
        }, f)
    print(f"\nCompleted! Fetched {len(_extracted_names)} names total.")
    
    return _extracted_names

def generate_index():
    """Scan games directory and generate JSON index of all supported games with names."""
    global _current_app_map
    games_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')
    
    # Load existing names from games_index.json first (to avoid re-fetching)
    existing_names = load_existing_games_index()
    
    # Get mapping of AppID -> Name from Steam API
    app_map = get_steam_app_map()
    
    # Merge: existing names take precedence (already verified)
    # Then Steam API fills in the rest
    combined_map = {**app_map, **existing_names}
    _current_app_map = combined_map  # Update global for signal handler
    
    print(f"Combined map has {len(combined_map)} game names")
    
    games_list = []
    missing_ids = []
    
    if os.path.exists(games_dir):
        # First pass: collect IDs and check what we have
        for filename in os.listdir(games_dir):
            if filename.endswith('.lua'):
                app_id = filename[:-4]
                
                # Check if we have the name
                if app_id not in combined_map:
                    missing_ids.append(app_id)

    # Refined Logic for Fallback
    if missing_ids:
        print(f"Found {len(missing_ids)} apps without names. Fetching all...")
        
        # Prioritize Forza Horizon 4 if missing
        ids_to_fetch = []
        if "1293830" in missing_ids:
            ids_to_fetch.append("1293830")
            missing_ids.remove("1293830")
        
        # Fetch ALL missing apps (no limit)
        ids_to_fetch.extend(missing_ids)
            
        fetched_names = fetch_names_from_store_api(ids_to_fetch)
        combined_map.update(fetched_names)
        _current_app_map = combined_map # Update global again
    else:
        print("No missing game names to fetch!")

    # Build and save final list using helper
    save_games_index(combined_map)
    
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games_index.json')

if __name__ == '__main__':
    generate_index()