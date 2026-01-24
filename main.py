"""
Steam Lua Patcher - Clean Modern Edition
A compact, card-based UI built with PyQt6
"""

import os
import shutil
import subprocess
import sys
import json
import time
from typing import Optional

from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QSize
)
from PyQt6.QtGui import (
    QFont, QColor, QPainter, QIcon, QPixmap
)
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QListWidget, QListWidgetItem,
    QMessageBox, QProgressBar, QFrame, QGraphicsDropShadowEffect,
    QAbstractItemView, QStackedWidget
)
from PyQt6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt6.QtCore import QUrl


def get_resource_path(relative_path):
    """Get absolute path to resource, works for dev and for PyInstaller"""
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, relative_path)


# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION - UPDATE AFTER DEPLOYMENT
# ═══════════════════════════════════════════════════════════════════════════════

# Webserver URL - Update after deploying to Vercel
WEBSERVER_BASE_URL = "https://webserver-ecru.vercel.app"

# URLs for fetching data (both use the same webserver)
GAMES_INDEX_URL = f"{WEBSERVER_BASE_URL}/api/games_index.json"
LUA_FILE_URL = f"{WEBSERVER_BASE_URL}/lua/"

# Local cache directory
LOCAL_CACHE_DIR = os.path.join(os.getenv('APPDATA', os.path.expanduser('~')), 'SteamLuaPatcher')
LOCAL_INDEX_PATH = os.path.join(LOCAL_CACHE_DIR, 'games_index.json')


# ═══════════════════════════════════════════════════════════════════════════════
# COLOR PALETTE
# ═══════════════════════════════════════════════════════════════════════════════

class Colors:
    BG_DARK = "#0d1117"
    BG_CARD = "#161b22"
    BG_CARD_HOVER = "#1c2128"
    ACCENT_BLUE = "#4f8cff"
    ACCENT_PURPLE = "#a855f7"
    ACCENT_GREEN = "#22c55e"
    TEXT_PRIMARY = "#e6edf3"
    TEXT_SECONDARY = "#8b949e"
    BORDER = "#30363d"


# ═══════════════════════════════════════════════════════════════════════════════
# STYLESHEET
# ═══════════════════════════════════════════════════════════════════════════════

STYLESHEET = f"""
QMainWindow {{
    background-color: {Colors.BG_DARK};
}}

QWidget {{
    background-color: transparent;
    color: {Colors.TEXT_PRIMARY};
    font-family: 'Segoe UI', sans-serif;
}}

QLabel {{
    background: transparent;
}}

QLineEdit {{
    background-color: {Colors.BG_CARD};
    border: 1px solid {Colors.BORDER};
    border-radius: 8px;
    padding: 10px 14px;
    font-size: 13px;
    color: {Colors.TEXT_PRIMARY};
}}

QLineEdit:focus {{
    border: 1px solid {Colors.ACCENT_BLUE};
}}

QListWidget {{
    background-color: {Colors.BG_CARD};
    border: 1px solid {Colors.BORDER};
    border-radius: 8px;
    padding: 4px;
    outline: none;
}}

QListWidget::item {{
    background-color: transparent;
    border-radius: 6px;
    padding: 8px;
    margin: 2px 0;
}}

QListWidget::item:selected {{
    background-color: {Colors.ACCENT_BLUE}30;
}}

QListWidget::item:hover {{
    background-color: {Colors.BG_CARD_HOVER};
}}

QScrollBar:vertical {{
    background-color: {Colors.BG_CARD};
    width: 8px;
    border-radius: 4px;
}}

QScrollBar::handle:vertical {{
    background-color: {Colors.BORDER};
    border-radius: 4px;
    min-height: 20px;
}}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0;
}}

QProgressBar {{
    background-color: {Colors.BG_CARD};
    border: none;
    border-radius: 4px;
    height: 4px;
    text-align: center;
}}

QProgressBar::chunk {{
    background-color: {Colors.ACCENT_BLUE};
    border-radius: 4px;
}}

QMessageBox {{
    background-color: {Colors.BG_CARD};
}}

QMessageBox QLabel {{
    color: {Colors.TEXT_PRIMARY};
}}

QMessageBox QPushButton {{
    background-color: {Colors.BG_CARD};
    border: 1px solid {Colors.BORDER};
    border-radius: 6px;
    padding: 8px 16px;
    min-width: 70px;
}}

QMessageBox QPushButton:hover {{
    background-color: {Colors.BG_CARD_HOVER};
}}
"""


# ═══════════════════════════════════════════════════════════════════════════════
# CONSTANTS
# ═══════════════════════════════════════════════════════════════════════════════

STEAM_PLUGIN_DIR = r"C:\Program Files (x86)\Steam\config\stplug-in"
STEAM_EXE_PATH = r"C:\Program Files (x86)\Steam\Steam.exe"


# ═══════════════════════════════════════════════════════════════════════════════
# WORKER THREADS
# ═══════════════════════════════════════════════════════════════════════════════

class IndexDownloadWorker(QThread):
    finished = pyqtSignal(set)
    progress = pyqtSignal(str)
    error = pyqtSignal(str)
    
    def run(self):
        try:
            # Move imports to top or keep local if needed, but clean logic
            import urllib.request
            import urllib.error
            
            self.progress.emit("Connecting...")
            
            if not os.path.exists(LOCAL_CACHE_DIR):
                os.makedirs(LOCAL_CACHE_DIR)
            
            try:
                self.progress.emit("Syncing games...")
                
                req = urllib.request.Request(
                    GAMES_INDEX_URL,
                    headers={'User-Agent': 'SteamLuaPatcher/2.0'}
                )
                
                with urllib.request.urlopen(req, timeout=30) as response:
                    data = response.read().decode('utf-8')
                    index_data = json.loads(data)
                
                with open(LOCAL_INDEX_PATH, 'w') as f:
                    json.dump(index_data, f)
                
            except (urllib.error.URLError, urllib.error.HTTPError):
                self.progress.emit("Using cached data...")
                if os.path.exists(LOCAL_INDEX_PATH):
                    with open(LOCAL_INDEX_PATH, 'r') as f:
                        index_data = json.load(f)
                else:
                    raise Exception("Check internet connection")
            
            app_ids = set(index_data.get('app_ids', []))
            self.finished.emit(app_ids)
            
        except Exception as e:
            self.error.emit(str(e))


class LuaDownloadWorker(QThread):
    finished = pyqtSignal(str)
    progress = pyqtSignal(int, int)
    status = pyqtSignal(str)
    error = pyqtSignal(str)
    
    def __init__(self, app_id: str):
        super().__init__()
        self.app_id = app_id
    
    def run(self):
        try:
            import urllib.request
            
            self.status.emit(f"Downloading patch...")
            
            url = f"{LUA_FILE_URL}{self.app_id}.lua"
            req = urllib.request.Request(url, headers={'User-Agent': 'SteamLuaPatcher/2.0'})
            cache_path = os.path.join(LOCAL_CACHE_DIR, f"{self.app_id}.lua")
            
            with urllib.request.urlopen(req, timeout=30) as response:
                total_size = response.getheader('Content-Length')
                total_size = int(total_size) if total_size else 0
                
                downloaded = 0
                data = b''
                chunk_size = 8192
                
                # Throttle progress updates: only emit every 1% or 200ms
                last_emit_time = 0
                import time
                
                while True:
                    chunk = response.read(chunk_size)
                    if not chunk:
                        break
                    data += chunk
                    downloaded += len(chunk)
                    
                    # Throttle signals to main thread to prevent UI freeze
                    current_time = time.time()
                    if total_size > 0:
                        if (current_time - last_emit_time) > 0.1:  # Max 10 updates per second
                            self.progress.emit(downloaded, total_size)
                            last_emit_time = current_time
            
            with open(cache_path, 'wb') as f:
                f.write(data)
            
            self.finished.emit(cache_path)
            
        except Exception as e:
            self.error.emit(str(e))


class RestartWorker(QThread):
    finished = pyqtSignal(str)
    error = pyqtSignal(str)
    
    def run(self):
        try:
            subprocess.run("taskkill /F /IM steam.exe", shell=True, 
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(2)
            
            if os.path.exists(STEAM_EXE_PATH):
                subprocess.Popen([STEAM_EXE_PATH])
                self.finished.emit("Steam restarted!")
            else:
                subprocess.run("start steam://open/main", shell=True)
                self.finished.emit("Steam restart sent.")
        except Exception as e:
            self.error.emit(str(e))


# ═══════════════════════════════════════════════════════════════════════════════
# CUSTOM WIDGETS
# ═══════════════════════════════════════════════════════════════════════════════

class ActionButton(QPushButton):
    """Compact action button with icon"""
    
    def __init__(self, icon_char: str, title: str, accent_color: str, parent=None):
        super().__init__(f"{icon_char}  {title}", parent)
        self.accent_color = accent_color
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setFixedHeight(42)
        self.setStyleSheet(f"""
            QPushButton {{
                background-color: {Colors.BG_CARD};
                border: 1px solid {Colors.BORDER};
                border-radius: 8px;
                font-size: 13px;
                font-weight: 500;
                color: {Colors.TEXT_PRIMARY};
                padding: 0 16px;
            }}
            QPushButton:hover {{
                background-color: {Colors.BG_CARD_HOVER};
                border: 1px solid {accent_color}60;
            }}
            QPushButton:pressed {{
                background-color: {accent_color}20;
            }}
            QPushButton:disabled {{
                color: {Colors.TEXT_SECONDARY};
                background-color: {Colors.BG_CARD};
            }}
        """)


from PyQt6.QtCore import QPropertyAnimation, QEasingCurve

class LoadingSpinner(QLabel):
    """Smoother loading indicator using QPropertyAnimation"""
    def __init__(self, parent=None):
        super().__init__("⟳", parent)
        self.setStyleSheet(f"font-size: 28px; color: {Colors.ACCENT_BLUE}; font-weight: bold;")
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setFixedSize(40, 40)
        
        # Setup modern layout based rotation
        self.timer = QTimer()
        self.timer.timeout.connect(self._rotate)
        self.angle = 0
        
    def _rotate(self):
        # Rotate text symbol
        self.angle = (self.angle + 1)
        # Use simpler but lighter animation
        chars = ["⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"]
        self.setText(chars[self.angle % len(chars)])
    
    def start(self):
        # 80ms interval is smooth (12fps) but light
        self.timer.start(80)
        self.show()
    
    def stop(self):
        self.timer.stop()
        self.hide()


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN APPLICATION
# ═══════════════════════════════════════════════════════════════════════════════

class SteamPatcherApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Steam Lua Patcher")
        self.setFixedSize(420, 520)
        
        try:
            self.setWindowIcon(QIcon(get_resource_path("logo.ico")))
        except:
            pass
        
        self.cached_app_ids: set = set()
        self.search_results = []
        self.selected_game = None
        self.current_search_id = 0
        
        self.debounce_timer = QTimer()
        self.debounce_timer.setSingleShot(True)
        self.debounce_timer.timeout.connect(self.do_search)
        
        self.network_manager = QNetworkAccessManager()
        self.network_manager.finished.connect(self._on_search_finished)
        self.active_reply = None
        
        self._setup_ui()
        self._start_init()
    
    def _setup_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        
        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(24, 24, 24, 24)
        main_layout.setSpacing(20)
        
        # ═══════════ HEADER ═══════════
        header = QVBoxLayout()
        header.setSpacing(4)
        header.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        # Icon
        icon_frame = QFrame()
        icon_frame.setFixedSize(56, 56)
        icon_frame.setStyleSheet(f"""
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                stop:0 {Colors.ACCENT_BLUE}, stop:1 {Colors.ACCENT_PURPLE});
            border-radius: 14px;
        """)
        icon_layout = QVBoxLayout(icon_frame)
        icon_layout.setContentsMargins(0, 0, 0, 0)
        icon_label = QLabel("⚡")
        icon_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        icon_label.setStyleSheet("font-size: 26px;")
        icon_layout.addWidget(icon_label)
        
        header.addWidget(icon_frame, alignment=Qt.AlignmentFlag.AlignCenter)
        header.addSpacing(12)
        
        title = QLabel("Steam Lua Patcher")
        title.setStyleSheet(f"font-size: 22px; font-weight: bold; color: {Colors.TEXT_PRIMARY};")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header.addWidget(title)
        
        self.status_label = QLabel("Loading...")
        self.status_label.setStyleSheet(f"font-size: 13px; color: {Colors.TEXT_SECONDARY};")
        self.status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header.addWidget(self.status_label)
        
        main_layout.addLayout(header)
        
        # ═══════════ STACKED WIDGET ═══════════
        self.stack = QStackedWidget()
        main_layout.addWidget(self.stack, 1)
        
        # Loading page
        self.loading_page = QWidget()
        loading_layout = QVBoxLayout(self.loading_page)
        loading_layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.spinner = LoadingSpinner()
        loading_layout.addWidget(self.spinner)
        self.stack.addWidget(self.loading_page)
        
        # Main page
        self.main_page = QWidget()
        main_page_layout = QVBoxLayout(self.main_page)
        main_page_layout.setContentsMargins(0, 0, 0, 0)
        main_page_layout.setSpacing(12)
        
        # Search
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("🔍  Search for a game...")
        self.search_input.textChanged.connect(self.on_search_change)
        main_page_layout.addWidget(self.search_input)
        
        # Results list
        self.results_list = QListWidget()
        self.results_list.setUniformItemSizes(True)  # Performance optimization
        self.results_list.setBatchSize(100)
        self.results_list.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        # Use itemPressed for immediate visual feedback
        self.results_list.itemPressed.connect(self.on_game_selected)
        # Keep currentItemChanged for keyboard navigation
        self.results_list.currentItemChanged.connect(self.on_game_selected)
        
        # Local stylesheet for faster repaints (prevents global recalc)
        self.results_list.setStyleSheet(f"""
            QListWidget {{
                background-color: {Colors.BG_CARD};
                border: 1px solid {Colors.BORDER};
                border-radius: 8px;
                padding: 4px;
                outline: none;
            }}
            QListWidget::item {{
                background-color: transparent;
                border-radius: 6px;
                padding: 8px;
                margin: 2px 0;
            }}
            QListWidget::item:selected {{
                background-color: {Colors.ACCENT_GREEN}40;  /* Green tint for selection */
                border: 1px solid {Colors.ACCENT_GREEN};
            }}
            QListWidget::item:hover {{
                background-color: {Colors.BG_CARD_HOVER};
            }}
        """)
        
        main_page_layout.addWidget(self.results_list, 1)
        
        # Action buttons (horizontal)
        buttons_layout = QHBoxLayout()
        buttons_layout.setSpacing(10)
        
        self.patch_btn = ActionButton("⬇", "Patch Game", Colors.ACCENT_BLUE)
        self.patch_btn.clicked.connect(self.do_patch)
        self.patch_btn.setEnabled(False)
        buttons_layout.addWidget(self.patch_btn)
        
        self.restart_btn = ActionButton("↻", "Restart Steam", Colors.ACCENT_PURPLE)
        self.restart_btn.clicked.connect(self.do_restart)
        buttons_layout.addWidget(self.restart_btn)
        
        main_page_layout.addLayout(buttons_layout)
        
        self.stack.addWidget(self.main_page)
        
        # Progress bar
        self.progress = QProgressBar()
        self.progress.setFixedHeight(4)
        self.progress.setTextVisible(False)
        self.progress.hide()
        main_layout.addWidget(self.progress)
    
    def _start_init(self):
        self.stack.setCurrentWidget(self.loading_page)
        self.spinner.start()
        self.status_label.setText("Syncing with server...")
        
        self.init_worker = IndexDownloadWorker()
        self.init_worker.finished.connect(self._on_init_done)
        self.init_worker.progress.connect(lambda s: self.status_label.setText(s))
        self.init_worker.error.connect(self._on_init_error)
        self.init_worker.start()
    
    def _on_init_done(self, app_ids):
        self.cached_app_ids = app_ids
        self.spinner.stop()
        self.stack.setCurrentWidget(self.main_page)
        self.status_label.setText(f"Ready • {len(app_ids):,} games available")
        self.search_input.setFocus()
    
    def _on_init_error(self, error):
        self.spinner.stop()
        self.status_label.setText(f"Error: {error}")
        QMessageBox.critical(self, "Error", f"Failed to load:\n{error}")
    
    def on_search_change(self, text):
        self.debounce_timer.stop()
        if text.strip():
            self.debounce_timer.start(400)
        else:
            self.results_list.clear()
    
    def do_search(self):
        query = self.search_input.text().strip()
        if not query:
            return
        
        self.current_search_id += 1
        self.status_label.setText(f"Searching...")
        
        if self.active_reply:
            self.active_reply.abort()
        
        from PyQt6.QtCore import QUrlQuery
        url = QUrl("https://store.steampowered.com/api/storesearch")
        q = QUrlQuery()
        q.addQueryItem("term", query)
        q.addQueryItem("l", "english")
        q.addQueryItem("cc", "US")
        url.setQuery(q)
        
        request = QNetworkRequest(url)
        self.active_reply = self.network_manager.get(request)
        self.active_reply.setProperty("search_id", self.current_search_id)
    
    def _on_search_finished(self, reply: QNetworkReply):
        reply.deleteLater()
        self.active_reply = None
        
        if reply.error() == QNetworkReply.NetworkError.OperationCanceledError:
            return
        
        if reply.property("search_id") != self.current_search_id:
            return
        
        if reply.error() != QNetworkReply.NetworkError.NoError:
            self.status_label.setText("Search failed")
            return
        
        try:
            data = json.loads(reply.readAll().data().decode("utf-8"))
            items = data.get("items", [])
            self._show_results(items)
        except:
            self.status_label.setText("Parse error")
    
    def _show_results(self, items):
        self.results_list.clear()
        self.search_results = items
        self.selected_game = None
        self.patch_btn.setEnabled(False)
        
        if not items:
            self.status_label.setText("No results found")
            return
        
        for item in items:
            name = item.get("name", "Unknown")
            appid = str(item.get("id", ""))
            available = appid in self.cached_app_ids
            
            status = "✓" if available else "✗"
            
            list_item = QListWidgetItem(f"{status}  {name}")
            list_item.setData(Qt.ItemDataRole.UserRole, {"name": name, "appid": appid, "available": available})
            
            if available:
                # Green background for available games (subtle tint)
                list_item.setBackground(QColor(Colors.ACCENT_GREEN).lighter(160))
                # Darker text for readability on green background? Or keep primary.
                # Actually, lighter(160) of #22c55e (green) is very light green.
                # Let's use a explicit semi-transparent green color for background to look like a "fill box"
                # And keep text readable.
                c = QColor(Colors.ACCENT_GREEN)
                c.setAlpha(40) # 40/255 opacity
                list_item.setBackground(c)
                list_item.setForeground(QColor(Colors.ACCENT_GREEN))
            else:
                # Red text for unavailable
                list_item.setForeground(QColor("#ff4444"))
                
            self.results_list.addItem(list_item)
        
        self.status_label.setText(f"{len(items)} results")
    
    def on_game_selected(self, current, previous=None):
        if not current:
            self.selected_game = None
            self.patch_btn.setEnabled(False)
            self.status_label.setText("Select a game")
            return
            
        data = current.data(Qt.ItemDataRole.UserRole)
        if data and data.get("available"):
            self.selected_game = data
            self.patch_btn.setEnabled(True)
            self.status_label.setText(f"Selected: {data['name']}")
        else:
            self.selected_game = None
            self.patch_btn.setEnabled(False)
            self.status_label.setText("Patch not available for this game")
    
    def do_patch(self):
        if not self.selected_game:
            return
        
        self.patch_btn.setEnabled(False)
        self.progress.setMinimum(0)
        self.progress.setMaximum(100)
        self.progress.setValue(0)
        self.progress.show()
        
        self.download_worker = LuaDownloadWorker(self.selected_game["appid"])
        self.download_worker.finished.connect(self._on_download_done)
        self.download_worker.progress.connect(self._on_download_progress)
        self.download_worker.status.connect(lambda s: self.status_label.setText(s))
        self.download_worker.error.connect(self._on_download_error)
        self.download_worker.start()
    
    def _on_download_progress(self, downloaded, total):
        if total > 0:
            self.progress.setValue(int((downloaded / total) * 100))
    
    def _on_download_done(self, cache_path):
        try:
            appid = self.selected_game["appid"]
            name = self.selected_game["name"]
            dest = os.path.join(STEAM_PLUGIN_DIR, f"{appid}.lua")
            
            if not os.path.exists(STEAM_PLUGIN_DIR):
                os.makedirs(STEAM_PLUGIN_DIR)
            
            shutil.copy2(cache_path, dest)
            
            self.progress.hide()
            self.patch_btn.setEnabled(True)
            self.status_label.setText(f"✓ Patched: {name}")
            
            QMessageBox.information(self, "Success", f"Patched {name}!\n\nRestart Steam to apply.")
            
        except Exception as e:
            self._on_download_error(str(e))
    
    def _on_download_error(self, error):
        self.progress.hide()
        self.patch_btn.setEnabled(True)
        self.status_label.setText(f"Error: {error}")
        QMessageBox.critical(self, "Error", f"Failed:\n{error}")
    
    def do_restart(self):
        reply = QMessageBox.question(
            self, "Restart Steam?",
            "This will close Steam and all games.\n\nContinue?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply != QMessageBox.StandardButton.Yes:
            return
        
        self.status_label.setText("Restarting Steam...")
        
        self.restart_worker = RestartWorker()
        self.restart_worker.finished.connect(lambda s: self.status_label.setText(s))
        self.restart_worker.error.connect(lambda e: self.status_label.setText(f"Error: {e}"))
        self.restart_worker.start()


# ═══════════════════════════════════════════════════════════════════════════════
# ENTRY POINT
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    app.setStyleSheet(STYLESHEET)
    
    font = QFont("Segoe UI", 10)
    app.setFont(font)
    
    window = SteamPatcherApp()
    window.show()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
