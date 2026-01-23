import os
import shutil
import subprocess
import threading
import tkinter as tk
from tkinter import ttk, messagebox
import requests

# Constants
LUA_FILES_DIR = r"d:\antigravity\luapatcher\All Games Files"
STEAM_PLUGIN_DIR = r"C:\Program Files (x86)\Steam\config\stplug-in"
STEAM_EXE_PATH = r"C:\Program Files (x86)\Steam\Steam.exe"

class SteamPatcherApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Steam Lua Patcher")
        self.root.geometry("600x500")
        
        # Style
        style = ttk.Style()
        style.theme_use('clam')
        
        # UI Elements
        self.create_widgets()
        
        # Data
        self.search_results = []
        
    def create_widgets(self):
        # Search Frame
        search_frame = ttk.LabelFrame(self.root, text="Search Game", padding=(10, 10))
        search_frame.pack(fill="x", padx=10, pady=5)
        
        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", self.on_search_change)
        self.search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        self.search_entry.pack(side="left", fill="x", expand=True, padx=5)
        # self.search_entry.bind("<Return>", lambda e: self.start_search()) # Enter key still works but not strictly needed with auto-search
        
        search_btn = ttk.Button(search_frame, text="Search", command=self.start_search)
        search_btn.pack(side="right", padx=5)
        
        # Debounce timer
        self.debounce_timer = None
        
        # Results Frame
        results_frame = ttk.LabelFrame(self.root, text="Results", padding=(10, 10))
        results_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        columns = ("name", "appid", "status")
        self.tree = ttk.Treeview(results_frame, columns=columns, show="headings", selectmode="browse")
        self.tree.heading("name", text="Game Name")
        self.tree.heading("appid", text="App ID")
        self.tree.heading("status", text="Lua File Status")
        self.tree.column("name", width=300)
        self.tree.column("appid", width=100)
        self.tree.column("status", width=120)
        self.tree.pack(fill="both", expand=True, side="left")
        
        scrollbar = ttk.Scrollbar(results_frame, orient="vertical", command=self.tree.yview)
        scrollbar.pack(side="right", fill="y")
        self.tree.configure(yscrollcommand=scrollbar.set)
        
        # self.tree.bind("<<TreeviewSelect>>", self.on_select)
        
        # Actions Frame
        actions_frame = ttk.Frame(self.root, padding=(10, 10))
        actions_frame.pack(fill="x", padx=10, pady=5)
        
        self.patch_btn = ttk.Button(actions_frame, text="Patch (Copy Lua)", command=self.patch_selected)
        self.patch_btn.pack(side="left", padx=5)
        
        self.restart_btn = ttk.Button(actions_frame, text="Restart Steam", command=self.restart_steam)
        self.restart_btn.pack(side="right", padx=5)
        
        # Status Bar
        self.status_var = tk.StringVar(value="Ready")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief="sunken", anchor="w")
        status_bar.pack(fill="x", side="bottom")

    def on_search_change(self, *args):
        if self.debounce_timer:
            self.root.after_cancel(self.debounce_timer)
        self.debounce_timer = self.root.after(600, self.start_search)

    def start_search(self):
        query = self.search_var.get().strip()
        if not query:
            return
        
        # Debounce cleanup if called manually
        if self.debounce_timer:
            self.root.after_cancel(self.debounce_timer)
            self.debounce_timer = None
        
        self.patch_btn.config(state="disabled")
        self.status_var.set("Searching...")
        self.tree.delete(*self.tree.get_children())
        
        # Run in thread to not freeze UI
        threading.Thread(target=self.search_logic, args=(query,), daemon=True).start()

    def search_logic(self, query):
        try:
            url = "https://store.steampowered.com/api/storesearch"
            params = {
                "term": query,
                "l": "english",
                "cc": "US"
            }
            response = requests.get(url, params=params)
            response.raise_for_status()
            data = response.json()
            
            items = data.get("items", [])
            self.root.after(0, self.update_results, items)
            
        except Exception as e:
            self.root.after(0, lambda: self.status_var.set(f"Error: {e}"))

    def update_results(self, items):
        self.search_results = items
        for item in items:
            name = item.get("name")
            appid = item.get("id")
            
            # Check if lua file exists
            lua_path = os.path.join(LUA_FILES_DIR, f"{appid}.lua")
            status = "Found" if os.path.exists(lua_path) else "Not Found"
            
            self.tree.insert("", "end", values=(name, appid, status))
            
        self.status_var.set(f"Found {len(items)} results.")
        self.patch_btn.config(state="normal")

    def patch_selected(self):
        selected = self.tree.selection()
        if not selected:
            messagebox.showwarning("No Selection", "Please select a game to patch.")
            return
        
        item_values = self.tree.item(selected[0])['values']
        name = item_values[0]
        appid = str(item_values[1])
        status = item_values[2]
        
        if status != "Found":
            messagebox.showerror("Error", f"Lua file for '{name}' (AppID: {appid}) not found in repository.")
            return
            
        src_file = os.path.join(LUA_FILES_DIR, f"{appid}.lua")
        dest_file = os.path.join(STEAM_PLUGIN_DIR, f"{appid}.lua")
        
        try:
            # Ensure destination dir exists
            if not os.path.exists(STEAM_PLUGIN_DIR):
                os.makedirs(STEAM_PLUGIN_DIR)
                
            shutil.copy2(src_file, dest_file)
            messagebox.showinfo("Success", f"Patched '{name}' successfully!\nCopied to: {dest_file}")
            self.status_var.set(f"Patched {name}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to copy file: {e}")

    def restart_steam(self):
        if not messagebox.askyesno("Confirm Restart", "This will close Steam and all running games. Continue?"):
            return
            
        self.status_var.set("Restarting Steam...")
        
        def restart_thread():
            try:
                # Kill Steam
                subprocess.run("taskkill /F /IM steam.exe", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                
                # Wait a bit
                import time
                time.sleep(2)
                
                # Start Steam
                if os.path.exists(STEAM_EXE_PATH):
                    subprocess.Popen([STEAM_EXE_PATH])
                    self.root.after(0, lambda: self.status_var.set("Steam restarted."))
                else:
                    # Try protocol handler
                    subprocess.run("start steam://open/main", shell=True)
                    self.root.after(0, lambda: self.status_var.set("Steam restart command sent."))
                    
            except Exception as e:
                self.root.after(0, lambda: messagebox.showerror("Error", f"Failed to restart Steam: {e}"))

        threading.Thread(target=restart_thread, daemon=True).start()

if __name__ == "__main__":
    # Check dependencies check
    try:
        import requests
    except ImportError:
        messagebox.showerror("Error", "Missing 'requests' library. Please run 'pip install requests'")
        sys.exit(1)

    root = tk.Tk()
    app = SteamPatcherApp(root)
    root.mainloop()
