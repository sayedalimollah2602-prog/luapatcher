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
from concurrent.futures import ThreadPoolExecutor, as_completed
import threading

# Get Steam API key from environment variable (optional but recommended)
STEAM_API_KEY = os.environ.get('STEAM_API_KEY', '')

# Maximum runtime in seconds (5 hours)
MAX_RUNTIME_SECONDS = 5 * 60 * 60

# === SPEED SETTINGS ===
MAX_WORKERS = 10          # Number of parallel requests (increase for faster, decrease if rate limited)
REQUEST_DELAY = 0.1       # Delay between batches (seconds) - reduce for faster
BATCH_SIZE = 10           # How many to process before small delay

# Global variables for graceful shutdown on CTRL+C
_progress_file = None
_extracted_names = {}
_completed_ids = []
_current_app_map = {}
_lock = threading.Lock()  # Thread-safe access to shared data
_stop_flag = False        # Flag to stop all threads

def save_games_index(app_map):
    """Generate and write the games_index.json file."""
    games_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')
    fix_files_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'game-fix-files')
    games_list = []
    
    # Get list of available fix files
    fix_files = set()
    if os.path.exists(fix_files_dir):
        for filename in os.listdir(fix_files_dir):
            if filename.endswith('.zip'):
                fix_files.add(filename[:-4])  # Remove .zip extension
    
    if fix_files:
        print(f"Found {len(fix_files)} game fix files: {fix_files}")
    
    if os.path.exists(games_dir):
        for filename in os.listdir(games_dir):
            if filename.endswith('.lua'):
                app_id = filename[:-4]
                name = app_map.get(app_id, f"Unknown Game ({app_id})")
                
                games_list.append({
                    "id": app_id,
                    "name": name,
                    "has_fix": app_id in fix_files
                })
    
    games_list.sort(key=lambda x: x['name'])
    
    index_data = {
        'games': games_list,
        'count': len(games_list),
        'last_updated': int(time.time())
    }
    
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
    global _stop_flag
    _stop_flag = True
    print(f"\n\n=== INTERRUPTED (CTRL+C) ===")
    
    if _progress_file and (_extracted_names or _completed_ids):
        print("Saving fetch progress...")
        try:
            with _lock:
                with open(_progress_file, 'w') as f:
                    json.dump({
                        'names': _extracted_names,
                        'completed_ids': _completed_ids
                    }, f)
            print(f"Saved {len(_extracted_names)} names to {_progress_file}")
        except Exception as e:
            print(f"Error saving progress: {e}")

    print("Updating games_index.json with collected data...")
    try:
        final_map = _current_app_map.copy()
        final_map.update(_extracted_names)
        save_games_index(final_map)
    except Exception as e:
        print(f"Error updating index: {e}")

    print("Exiting...")
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def load_existing_games_index():
    """Load existing games_index.json to avoid re-fetching known games."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    index_path = os.path.join(script_dir, 'games_index.json')
    
    if os.path.exists(index_path):
        try:
            with open(index_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
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
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
    }
    
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
            display_url = url.split('?')[0] + ('?key=***' if 'key=' in url else '')
            print(f"Trying {display_url}...")
            response = requests.get(url, headers=headers, timeout=30)
            if response.status_code == 200:
                data = response.json()
                app_map = {}
                
                apps = []
                if 'applist' in data and 'apps' in data['applist']:
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
        except Exception as e:
            print(f"Error fetching from {url}: {e}")
            
    print("All Steam App List URLs failed.")
    return {}

def fetch_single_app(app_id):
    """Fetch a single app's name from multiple sources. Returns (app_id, name or None)."""
    global _stop_flag
    
    if _stop_flag:
        return (app_id, None)
    
    sources = [
        ("Store", f"https://store.steampowered.com/api/appdetails?appids={app_id}"),
        ("SteamSpy", f"https://steamspy.com/api.php?request=appdetails&appid={app_id}")
    ]
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
    }
    
    for source_name, url in sources:
        if _stop_flag:
            return (app_id, None)
            
        try:
            response = requests.get(url, headers=headers, timeout=5)  # Reduced timeout
            
            if response.status_code == 200:
                data = response.json()
                
                if source_name == "Store":
                    if data and str(app_id) in data:
                        details = data[str(app_id)]
                        if details.get("success") and "data" in details:
                            return (app_id, details["data"]["name"])
                            
                elif source_name == "SteamSpy":
                    if data and "name" in data:
                        return (app_id, data["name"])
                        
            elif response.status_code == 429:
                time.sleep(2)  # Reduced wait on rate limit
                
        except Exception:
            pass
    
    return (app_id, None)

def fetch_names_from_store_api(app_ids):
    """Fetch specific app names using parallel requests."""
    global _progress_file, _extracted_names, _completed_ids, _stop_flag
    
    if not app_ids:
        return {}
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    _progress_file = os.path.join(script_dir, 'fetch_progress.json')
    
    # Load existing progress
    _extracted_names = {}
    if os.path.exists(_progress_file):
        try:
            with open(_progress_file, 'r') as f:
                progress_data = json.load(f)
                _extracted_names = progress_data.get('names', {})
                completed_ids_set = set(progress_data.get('completed_ids', []))
                remaining_ids = [aid for aid in app_ids if aid not in completed_ids_set]
                print(f"Resuming from progress file. Already fetched: {len(completed_ids_set)} names.")
                app_ids = remaining_ids
        except Exception as e:
            print(f"Could not load progress file: {e}")
    
    if not app_ids:
        print("All apps already fetched from previous run.")
        return _extracted_names
    
    total = len(app_ids)
    print(f"\n{'='*60}")
    print(f"FAST PARALLEL FETCHING: {total} games")
    print(f"Workers: {MAX_WORKERS} | Delay: {REQUEST_DELAY}s | Batch: {BATCH_SIZE}")
    print(f"Estimated time: ~{total / MAX_WORKERS * 0.5 / 60:.1f} minutes")
    print(f"{'='*60}\n")
    
    _completed_ids = list(_extracted_names.keys())
    start_time = time.time()
    completed_count = 0
    success_count = 0
    save_interval = 50
    
    # Use ThreadPoolExecutor for parallel requests
    with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        # Submit all tasks
        future_to_id = {executor.submit(fetch_single_app, app_id): app_id for app_id in app_ids}
        
        for future in as_completed(future_to_id):
            if _stop_flag:
                break
                
            app_id = future_to_id[future]
            completed_count += 1
            
            try:
                result_id, name = future.result()
                
                with _lock:
                    _completed_ids.append(result_id)
                    
                    if name:
                        _extracted_names[str(result_id)] = name
                        success_count += 1
                        status = f"✓ {name[:40]}"
                    else:
                        status = "✗ Failed"
                
                # Progress bar
                progress_pct = completed_count / total * 100
                elapsed = time.time() - start_time
                rate = completed_count / elapsed if elapsed > 0 else 0
                eta = (total - completed_count) / rate if rate > 0 else 0
                
                print(f"[{progress_pct:5.1f}%] {completed_count}/{total} | "
                      f"{rate:.1f}/s | ETA: {eta:.0f}s | {app_id}: {status}")
                
                # Save progress periodically
                if completed_count % save_interval == 0:
                    with _lock:
                        with open(_progress_file, 'w') as f:
                            json.dump({
                                'names': _extracted_names,
                                'completed_ids': _completed_ids
                            }, f)
                    print(f"--- Progress saved ({success_count} names) ---")
                
                # Check timeout
                if elapsed >= MAX_RUNTIME_SECONDS:
                    print(f"\n=== 5 HOUR TIMEOUT ===")
                    _stop_flag = True
                    break
                    
            except Exception as e:
                print(f"Error processing {app_id}: {e}")
    
    # Final save
    with _lock:
        with open(_progress_file, 'w') as f:
            json.dump({
                'names': _extracted_names,
                'completed_ids': _completed_ids
            }, f)
    
    elapsed = time.time() - start_time
    print(f"\n{'='*60}")
    print(f"COMPLETED in {elapsed:.1f}s ({elapsed/60:.1f} min)")
    print(f"Success: {success_count}/{completed_count} ({success_count/completed_count*100:.1f}%)")
    print(f"{'='*60}\n")
    
    return _extracted_names

def generate_index():
    """Scan games directory and generate JSON index."""
    global _current_app_map
    games_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')
    
    existing_names = load_existing_games_index()
    app_map = get_steam_app_map()
    
    combined_map = {**app_map, **existing_names}
    _current_app_map = combined_map
    
    print(f"Combined map has {len(combined_map)} game names")
    
    missing_ids = []
    
    if os.path.exists(games_dir):
        for filename in os.listdir(games_dir):
            if filename.endswith('.lua'):
                app_id = filename[:-4]
                if app_id not in combined_map:
                    missing_ids.append(app_id)

    if missing_ids:
        print(f"Found {len(missing_ids)} apps without names. Fetching...")
        
        ids_to_fetch = []
        if "1293830" in missing_ids:
            ids_to_fetch.append("1293830")
            missing_ids.remove("1293830")
        
        ids_to_fetch.extend(missing_ids)
        fetched_names = fetch_names_from_store_api(ids_to_fetch)
        combined_map.update(fetched_names)
        _current_app_map = combined_map
    else:
        print("No missing game names to fetch!")

    save_games_index(combined_map)
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games_index.json')

if __name__ == '__main__':
    generate_index()