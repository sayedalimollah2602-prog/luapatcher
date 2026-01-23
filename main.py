import os
import shutil
import subprocess
import threading
import tkinter as tk
from tkinter import messagebox
import sys
import time

try:
    import requests
    import ttkbootstrap as ttk
    from ttkbootstrap.constants import *
except ImportError:
    # Just in case dependencies aren't installed yet, fail gracefully or fallback
    import tkinter.ttk as ttk
    from tkinter import TOP, BOTTOM, LEFT, RIGHT, BOTH, X, Y, END

def get_resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    try:
        # PyInstaller creates a temp folder and stores path in _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.dirname(os.path.abspath(__file__))

    return os.path.join(base_path, relative_path)

# Constants
# If running as source, we might need to point to the folder specifically if it's not in the same dir as main.py
# But for now, let's assume if it's source, it's relative or absolute. 
# The user's original path was absolute: r"d:\antigravity\luapatcher\All Games Files"
# We need to switch logic: if bundled, use embedded; if source, use original known path OR relative.

if getattr(sys, 'frozen', False):
    # Running as compiled exe (bundled)
    LUA_FILES_DIR = get_resource_path("All Games Files")
else:
    # Running as script (relative to script)
    LUA_FILES_DIR = get_resource_path("All Games Files")

STEAM_PLUGIN_DIR = r"C:\Program Files (x86)\Steam\config\stplug-in"
STEAM_EXE_PATH = r"C:\Program Files (x86)\Steam\Steam.exe"

class SteamPatcherApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Steam Lua Patcher")
        self.root.geometry("800x600")
        
        # UI Elements
        self.create_widgets()
        
        # Data
        self.search_results = []
        self.debounce_timer = None
        self.current_search_id = 0
        
    def create_widgets(self):
        # Main Container with padding
        main_container = ttk.Frame(self.root, padding=20)
        main_container.pack(fill=BOTH, expand=True)
        
        # Header
        header_frame = ttk.Frame(main_container)
        header_frame.pack(fill=X, pady=(0, 20))
        
        title_lbl = ttk.Label(header_frame, text="Steam Lua Patcher", font=("Helvetica", 24, "bold"), bootstyle="primary")
        title_lbl.pack(side=LEFT)
        
        # Search Section
        search_frame = ttk.Labelframe(main_container, text="Game Search", padding=15, bootstyle="info")
        search_frame.pack(fill=X, pady=(0, 20))
        
        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", self.on_search_change)
        
        entry_frame = ttk.Frame(search_frame)
        entry_frame.pack(fill=X)
        
        search_icon_lbl = ttk.Label(entry_frame, text="🔍", font=("Segoe UI Emoji", 12))
        search_icon_lbl.pack(side=LEFT, padx=(0, 10))
        
        self.search_entry = ttk.Entry(entry_frame, textvariable=self.search_var, font=("Helvetica", 11))
        self.search_entry.pack(side=LEFT, fill=X, expand=True)
        self.search_entry.focus_set()
        
        # Results Section
        results_frame = ttk.Labelframe(main_container, text="Search Results", padding=15, bootstyle="default")
        results_frame.pack(fill=BOTH, expand=True, pady=(0, 20))
        
        columns = ("name", "appid", "status")
        self.tree = ttk.Treeview(results_frame, columns=columns, show="headings", selectmode="browse", bootstyle="info")
        
        self.tree.heading("name", text="Game Name")
        self.tree.heading("appid", text="App ID")
        self.tree.heading("status", text="Lua Patch Status")
        
        self.tree.column("name", width=400, anchor="w")
        self.tree.column("appid", width=100, anchor="center")
        self.tree.column("status", width=150, anchor="center")
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(results_frame, orient="vertical", command=self.tree.yview)
        scrollbar.pack(side=RIGHT, fill=Y)
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        self.tree.pack(fill=BOTH, expand=True, side=LEFT)
        self.tree.bind("<<TreeviewSelect>>", self.on_select)
        
        # Actions Section
        actions_frame = ttk.Frame(main_container)
        actions_frame.pack(fill=X)
        
        self.patch_btn = ttk.Button(actions_frame, text="Patch Selected Game", command=self.patch_selected, state="disabled", bootstyle="success-outline", width=25)
        self.patch_btn.pack(side=LEFT, padx=(0, 10))
        
        self.restart_btn = ttk.Button(actions_frame, text="Restart Steam", command=self.restart_steam, bootstyle="danger-outline", width=20)
        self.restart_btn.pack(side=RIGHT)

        # Footer / Status
        self.status_var = tk.StringVar(value="Ready to search")
        status_lbl = ttk.Label(self.root, textvariable=self.status_var, relief="sunken", anchor="w", padding=(10, 5), bootstyle="secondary-inverse")
        status_lbl.pack(fill=X, side=BOTTOM)

    def on_search_change(self, *args):
        if self.debounce_timer:
            self.root.after_cancel(self.debounce_timer)
        self.debounce_timer = self.root.after(400, self.start_search) # Reduced debounce to 400ms

    def start_search(self):
        query = self.search_var.get().strip()
        if not query:
            return
        
        if self.debounce_timer:
            self.root.after_cancel(self.debounce_timer)
            self.debounce_timer = None
            
        self.status_var.set(f"Searching for '{query}'...")
        
        # Increment search ID to handle race conditions
        self.current_search_id += 1
        search_id = self.current_search_id
        
        # Clear previous results immediately if new search starts? 
        # Optional: keeping old results until new ones arrive looks smoother.
        # self.tree.delete(*self.tree.get_children())
        
        threading.Thread(target=self.search_logic, args=(query, search_id), daemon=True).start()

    def search_logic(self, query, search_id):
        try:
            url = "https://store.steampowered.com/api/storesearch"
            params = {
                "term": query,
                "l": "english",
                "cc": "US"
            }
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            items = data.get("items", [])
            
            # Check if this thread is still relevant
            if search_id == self.current_search_id:
                self.root.after(0, self.update_results, items)
            else:
                print(f"Ignoring stale search result (ID: {search_id})")
            
        except requests.RequestException as e:
             if search_id == self.current_search_id:
                self.root.after(0, lambda: self.status_var.set(f"Network Error: {e}"))
        except Exception as e:
             if search_id == self.current_search_id:
                self.root.after(0, lambda: self.status_var.set(f"Error: {e}"))

    def update_results(self, items):
        self.tree.delete(*self.tree.get_children())
        self.search_results = items
        
        if not items:
            self.status_var.set("No results found.")
            return

        for item in items:
            name = item.get("name")
            appid = item.get("id")
            
            # Check if lua file exists
            lua_path = os.path.join(LUA_FILES_DIR, f"{appid}.lua")
            exists = os.path.exists(lua_path)
            status = "AVAILABLE" if exists else "Missing"
            
            # Insert with tags for coloring
            # We need to map boolean to a tag if using ttkbootstrap specific row colors, 
            # but simpler to just use text for now or configure tags.
            self.tree.insert("", "end", values=(name, appid, status), tags=("found" if exists else "missing",))
        
        # Configure tag colors (if standard ttk, bootstyle handles defaults differently)
        # self.tree.tag_configure("found", foreground="green") # bootstyle might override
        
        self.status_var.set(f"Found {len(items)} results.")
        
    def on_select(self, event):
        selected = self.tree.selection()
        if selected:
            item_values = self.tree.item(selected[0])['values']
            status = item_values[2] # "AVAILABLE" or "Missing"
            if status == "AVAILABLE":
                self.patch_btn.config(state="normal")
                self.status_var.set(f"Selected: {item_values[0]}")
            else:
                self.patch_btn.config(state="disabled")
                self.status_var.set(f"Lua file missing for: {item_values[0]}")
        else:
             self.patch_btn.config(state="disabled")

    def patch_selected(self):
        selected = self.tree.selection()
        if not selected:
            return
        
        item_values = self.tree.item(selected[0])['values']
        name = item_values[0]
        appid = str(item_values[1])
        
        src_file = os.path.join(LUA_FILES_DIR, f"{appid}.lua")
        dest_file = os.path.join(STEAM_PLUGIN_DIR, f"{appid}.lua")
        
        try:
            if not os.path.exists(STEAM_PLUGIN_DIR):
                os.makedirs(STEAM_PLUGIN_DIR)
                
            shutil.copy2(src_file, dest_file)
            messagebox.showinfo("Success", f"Patched '{name}' successfully!", parent=self.root)
            self.status_var.set(f"Successfully patched {name}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to copy file: {e}", parent=self.root)

    def restart_steam(self):
        if not messagebox.askyesno("Confirm Restart", "This will close Steam and all running games. Continue?", parent=self.root):
            return
            
        self.status_var.set("Restarting Steam...")
        
        def restart_thread():
            try:
                # Taskkill is reliable
                subprocess.run("taskkill /F /IM steam.exe", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                time.sleep(2)
                
                if os.path.exists(STEAM_EXE_PATH):
                    subprocess.Popen([STEAM_EXE_PATH])
                    self.root.after(0, lambda: self.status_var.set("Steam restarted."))
                else:
                    subprocess.run("start steam://open/main", shell=True)
                    self.root.after(0, lambda: self.status_var.set("Steam restart command sent."))
                    
            except Exception as e:
                self.root.after(0, lambda: messagebox.showerror("Error", f"Failed to restart Steam: {e}", parent=self.root))

        threading.Thread(target=restart_thread, daemon=True).start()

if __name__ == "__main__":
    # Theme setup
    try:
        root = ttk.Window(themename="darkly") # modern dark theme
    except NameError:
        root = tk.Tk()
        
    app = SteamPatcherApp(root)
    root.mainloop()
