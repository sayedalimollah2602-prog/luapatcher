"""
Steam Lua Patcher - Modern Glass Edition
A premium, dark-mode UI built with PyQt6
"""

import os
import shutil
import subprocess
import sys
import json
import time
from typing import Optional

from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QSize, QPropertyAnimation, 
    QEasingCurve, QPoint, QPointF, QRect, QRectF
)
from PyQt6.QtGui import (
    QFont, QColor, QPainter, QIcon, QPixmap, QLinearGradient, 
    QBrush, QPen, QPainterPath
)
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QListWidget, QListWidgetItem,
    QMessageBox, QProgressBar, QFrame, QGraphicsDropShadowEffect,
    QAbstractItemView, QStackedWidget, QGraphicsOpacityEffect
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
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

APP_VERSION = "1.0"
WEBSERVER_BASE_URL = "https://webserver-ecru.vercel.app"
GAMES_INDEX_URL = f"{WEBSERVER_BASE_URL}/api/games_index.json"
LUA_FILE_URL = f"{WEBSERVER_BASE_URL}/lua/"

LOCAL_CACHE_DIR = os.path.join(os.getenv('APPDATA', os.path.expanduser('~')), 'SteamLuaPatcher')
LOCAL_INDEX_PATH = os.path.join(LOCAL_CACHE_DIR, 'games_index.json')

STEAM_PLUGIN_DIR = r"C:\Program Files (x86)\Steam\config\stplug-in"
STEAM_EXE_PATH = r"C:\Program Files (x86)\Steam\Steam.exe"


# ═══════════════════════════════════════════════════════════════════════════════
# DESIGN SYSTEM
# ═══════════════════════════════════════════════════════════════════════════════

class Colors:
    # Deep Blue to Black Gradient
    BG_GRADIENT_START = "#0f1c30" 
    BG_GRADIENT_END = "#02040a"
    
    # Glass Components
    GLASS_BG = "rgba(30, 41, 59, 60)"       # Low opacity for glass feel
    GLASS_HOVER = "rgba(51, 65, 85, 80)"
    GLASS_BORDER = "rgba(255, 255, 255, 25)" # Subtle white border
    
    # Text
    TEXT_PRIMARY = "#FFFFFF"
    TEXT_SECONDARY = "#94A3B8"  # Slate 400
    
    # Accents
    ACCENT_BLUE = "#3B82F6"
    ACCENT_PURPLE = "#8B5CF6"
    ACCENT_GREEN = "#10B981"
    ACCENT_RED = "#EF4444"


STYLESHEET = f"""
QMainWindow {{
    background: transparent;
}}

QWidget {{
    font-family: 'Segoe UI', sans-serif;
    color: {Colors.TEXT_PRIMARY};
}}

/* Scrollbar */
QScrollBar:vertical {{
    background-color: transparent;
    width: 6px;
    margin: 0px;
}}
QScrollBar::handle:vertical {{
    background-color: {Colors.GLASS_BORDER};
    border-radius: 3px;
    min-height: 20px;
}}
QScrollBar::handle:vertical:hover {{
    background-color: {Colors.ACCENT_BLUE};
}}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0px;
}}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{
    background: none;
}}

/* List Widget */
QListWidget {{
    background-color: transparent;
    border: none;
    outline: none;
}}
QListWidget::item {{
    background-color: {Colors.GLASS_BG};
    border: 1px solid {Colors.GLASS_BORDER};
    border-radius: 12px;
    padding: 12px;
    margin-bottom: 8px;
    color: {Colors.TEXT_PRIMARY};
}}
QListWidget::item:hover {{
    background-color: {Colors.GLASS_HOVER};
    border: 1px solid {Colors.ACCENT_BLUE}80;
}}
QListWidget::item:selected {{
    background-color: {Colors.ACCENT_BLUE}20;
    border: 1px solid {Colors.ACCENT_BLUE};
}}

/* Inputs */
QLineEdit {{
    background-color: {Colors.GLASS_BG};
    border: 1px solid {Colors.GLASS_BORDER};
    border-radius: 12px;
    padding: 14px 16px;
    font-size: 14px;
    selection-background-color: {Colors.ACCENT_BLUE};
    color: {Colors.TEXT_PRIMARY};
}}
QLineEdit:focus {{
    border: 1px solid {Colors.ACCENT_BLUE};
    background-color: {Colors.GLASS_HOVER};
}}

/* MessageBox */
QMessageBox {{
    background-color: #1e293b;
}}
QMessageBox QLabel {{
    color: {Colors.TEXT_PRIMARY};
}}
QMessageBox QPushButton {{
    background-color: {Colors.ACCENT_BLUE};
    color: white;
    border-radius: 6px;
    padding: 6px 16px;
    border: none;
}}
"""


# ═══════════════════════════════════════════════════════════════════════════════
# UI COMPONENTS
# ═══════════════════════════════════════════════════════════════════════════════

class GlassButton(QPushButton):
    """
    Large horizontal action button with glassmorphism style.
    Contains: Icon (left), Title (bold), Description (muted).
    """
    def __init__(self, icon_char, title, description, accent_color, parent=None):
        super().__init__(parent)
        self.setFixedHeight(80)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        
        self.icon_char = icon_char
        self.title_text = title
        self.desc_text = description
        self.accent_color = accent_color
        
        # Opacity effect for disabled state
        self.opacity_effect = QGraphicsOpacityEffect(self)
        self.setGraphicsEffect(self.opacity_effect)
        self.opacity_effect.setOpacity(1.0)

        # Setup internal layout setup isn't needed for pure painting, 
        # but useful if we wanted child widgets. We'll use pure painting for performance and look.
    
    def setEnabled(self, enabled):
        super().setEnabled(enabled)
        self.opacity_effect.setOpacity(1.0 if enabled else 0.5)
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Determine State
        is_hover = self.underMouse() and self.isEnabled()
        is_pressed = self.isDown() and self.isEnabled()
        
        # Rects
        rect = self.rect()
        bg_rect = rect.adjusted(1, 1, -1, -1)
        
        # 1. Background (Glass)
        bg_color = QColor(Colors.GLASS_BG)
        if is_hover:
            bg_color = QColor(Colors.GLASS_HOVER)
        if is_pressed:
            bg_color = QColor(self.accent_color)
            bg_color.setAlpha(40)
            
        path = QPainterPath()
        path.addRoundedRect(QRectF(bg_rect), 16, 16)
        
        painter.fillPath(path, bg_color)
        
        # 2. Border (Subtle Gradient or Solid)
        border_color = QColor(Colors.GLASS_BORDER)
        if is_hover or is_pressed:
            border_color = QColor(self.accent_color)
            border_color.setAlpha(100)
            
        pen = QPen(border_color, 1)
        painter.setPen(pen)
        painter.drawPath(path)
        
        # 3. Inner Glow (Simplified as a soft gradient overlay if needed, skipping for cleaner look)
        
        # 4. Icon Box (Left)
        icon_size = 48
        icon_rect = QRect(20, (rect.height() - icon_size) // 2, icon_size, icon_size)
        
        # Icon Background Gradient
        grad = QLinearGradient(QPointF(icon_rect.topLeft()), QPointF(icon_rect.bottomRight()))
        c1 = QColor(self.accent_color)
        c2 = c1.darker(150)
        grad.setColorAt(0, c1)
        grad.setColorAt(1, c2)
        
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(grad))
        painter.drawRoundedRect(icon_rect, 12, 12)
        
        # Icon Text
        painter.setPen(QColor("#FFFFFF"))
        painter.setFont(QFont("Segoe UI Symbol", 18)) # Use Symbol font for icons
        painter.drawText(icon_rect, Qt.AlignmentFlag.AlignCenter, self.icon_char)
        
        # 5. Text Content
        text_x = 20 + icon_size + 16
        text_w = rect.width() - text_x - 10
        
        # Title
        title_rect = QRect(text_x, 18, text_w, 24)
        painter.setPen(QColor(Colors.TEXT_PRIMARY))
        painter.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        painter.drawText(title_rect, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter, self.title_text)
        
        # Description
        desc_rect = QRect(text_x, 42, text_w, 20)
        painter.setPen(QColor(Colors.TEXT_SECONDARY))
        painter.setFont(QFont("Segoe UI", 9))
        painter.drawText(desc_rect, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter, self.desc_text)


class LoadingSpinner(QLabel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(60, 60)
        self.setScaledContents(True)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.timer = QTimer()
        self.timer.timeout.connect(self._rotate)
        self.angle = 0
        
    def _rotate(self):
        self.angle = (self.angle + 30) % 360
        self.update()
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        center = QPoint(self.width() // 2, self.height() // 2)
        
        pen = QPen(QColor(Colors.ACCENT_BLUE), 4)
        pen.setCapStyle(Qt.PenCapStyle.RoundCap)
        painter.setPen(pen)
        
        # Draw arc
        rect = QRect(10, 10, 40, 40)
        start_angle = -self.angle * 16
        span_angle = 270 * 16
        painter.drawArc(rect, start_angle, span_angle)

    def start(self):
        self.timer.start(50)
        self.show()
    
    def stop(self):
        self.timer.stop()
        self.hide()


# ═══════════════════════════════════════════════════════════════════════════════
# WORKER THREADS (Preserved Logic)
# ═══════════════════════════════════════════════════════════════════════════════

class IndexDownloadWorker(QThread):
    finished = pyqtSignal(set)
    progress = pyqtSignal(str)
    error = pyqtSignal(str)
    
    def run(self):
        try:
            import urllib.request
            import urllib.error
            
            self.progress.emit("Connecting...")
            
            if not os.path.exists(LOCAL_CACHE_DIR):
                os.makedirs(LOCAL_CACHE_DIR)
            
            try:
                self.progress.emit("Syncing library...")
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
                self.progress.emit("Offline mode...")
                if os.path.exists(LOCAL_INDEX_PATH):
                    with open(LOCAL_INDEX_PATH, 'r') as f:
                        index_data = json.load(f)
                else:
                    raise Exception("Network error & no cache")
            
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
                total_size = int(response.getheader('Content-Length') or 0)
                downloaded = 0
                data = b''
                chunk_size = 8192
                last_emit = 0
                
                while True:
                    chunk = response.read(chunk_size)
                    if not chunk: break
                    data += chunk
                    downloaded += len(chunk)
                    
                    now = time.time()
                    if total_size > 0 and (now - last_emit) > 0.1:
                        self.progress.emit(downloaded, total_size)
                        last_emit = now
            
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
                self.finished.emit("Steam launched!")
            else:
                subprocess.run("start steam://open/main", shell=True)
                self.finished.emit("Restart command sent.")
        except Exception as e:
            self.error.emit(str(e))


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN WINDOW
# ═══════════════════════════════════════════════════════════════════════════════

class SteamPatcherApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Steam Lua Patcher")
        self.setFixedSize(900, 600) # Larger size for modern layout
        
        try:
            self.setWindowIcon(QIcon(get_resource_path("logo.ico")))
        except:
            pass
            
        self.cached_app_ids = set()
        self.selected_game = None
        
        # Search debounce
        self.debounce_timer = QTimer()
        self.debounce_timer.setSingleShot(True)
        self.debounce_timer.timeout.connect(self.do_search)
        self.current_search_id = 0
        
        # Network
        self.network_manager = QNetworkAccessManager()
        self.network_manager.finished.connect(self._on_search_finished)
        self.active_reply = None

        self._init_ui()
        self._start_sync()

    def paintEvent(self, event):
        """Paint the custom gradient background"""
        painter = QPainter(self)
        grad = QLinearGradient(0, 0, 0, self.height())
        grad.setColorAt(0, QColor(Colors.BG_GRADIENT_START))
        grad.setColorAt(1, QColor(Colors.BG_GRADIENT_END))
        painter.fillRect(self.rect(), grad)

    def _init_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QHBoxLayout(central)
        main_layout.setContentsMargins(40, 40, 40, 40)
        main_layout.setSpacing(40)
        
        # ─── LEFT COLUMN: ACTIONS (1/3 width) ───
        left_col = QVBoxLayout()
        left_col.setSpacing(16)
        
        # Header
        header_layout = QHBoxLayout()
        icon = QLabel("⚡")
        icon.setStyleSheet(f"font-size: 32px; color: {Colors.ACCENT_BLUE}; font-weight: bold;")
        title = QLabel("Lua Patcher")
        title.setStyleSheet(f"font-size: 24px; font-weight: 800; color: {Colors.TEXT_PRIMARY};")
        header_layout.addWidget(icon)
        header_layout.addWidget(title)
        header_layout.addStretch()
        left_col.addLayout(header_layout)
        
        left_col.addSpacing(20)
        
        # Status
        self.status_label = QLabel("Initializing...")
        self.status_label.setStyleSheet(f"color: {Colors.TEXT_SECONDARY}; font-size: 13px;")
        left_col.addWidget(self.status_label)
        
        left_col.addStretch()
        
        # Buttons
        self.btn_patch = GlassButton("⬇", "Patch Game", "Install Lua patch for selected game", Colors.ACCENT_GREEN)
        self.btn_patch.clicked.connect(self.do_patch)
        self.btn_patch.setEnabled(False) # Disabled until game selected
        left_col.addWidget(self.btn_patch)
        
        self.btn_restart = GlassButton("↻", "Restart Steam", "Apply changes by restarting Steam", Colors.ACCENT_PURPLE)
        self.btn_restart.clicked.connect(self.do_restart)
        left_col.addWidget(self.btn_restart)
        
        left_col.addStretch()
        
        # Version Label
        version_label = QLabel(f"v{APP_VERSION}")
        version_label.setStyleSheet(f"color: {Colors.TEXT_SECONDARY}; font-size: 12px; font-weight: bold;")
        version_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        left_col.addWidget(version_label)
        
        # Creator Footer
        creator_label = QLabel('<a href="https://github.com/sayedalimollah2602-prog" style="color: #94A3B8; text-decoration: none;">created by leVI</a>')
        creator_label.setStyleSheet(f"font-size: 11px;")
        creator_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        creator_label.setOpenExternalLinks(True)
        left_col.addWidget(creator_label)
        
        main_layout.addLayout(left_col, 35) # 35% width
        
        # ─── RIGHT COLUMN: SEARCH & LIST (2/3 width) ───
        right_col = QVBoxLayout()
        right_col.setSpacing(16)
        
        # Search Bar
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("Find a game...")
        self.search_input.textChanged.connect(self.on_search_change)
        
        # Add shadow to search
        shadow = QGraphicsDropShadowEffect(self.search_input)
        shadow.setBlurRadius(20)
        shadow.setColor(QColor(0, 0, 0, 80))
        shadow.setOffset(0, 4)
        self.search_input.setGraphicsEffect(shadow)
        
        right_col.addWidget(self.search_input)
        
        # Stack for Loading / List
        self.stack = QStackedWidget()
        
        # 1. Loading Page
        page_loading = QWidget()
        lay_loading = QVBoxLayout(page_loading)
        lay_loading.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.spinner = LoadingSpinner()
        lay_loading.addWidget(self.spinner)
        self.stack.addWidget(page_loading)
        
        # 2. Results Page
        self.results_list = QListWidget()
        self.results_list.setIconSize(QSize(40, 40))  # Big icons
        self.results_list.itemPressed.connect(self.on_game_selected)
        
        # Add scroll shadow? Maybe complex. Keep simple.
        
        self.stack.addWidget(self.results_list)
        
        right_col.addWidget(self.stack)
        
        # Progress Bar overlay (at bottom of right col)
        self.progress = QProgressBar()
        self.progress.setFixedHeight(4)
        self.progress.setTextVisible(False)
        self.progress.setStyleSheet(f"""
            QProgressBar {{
                background: {Colors.GLASS_BG};
                border-radius: 2px;
            }}
            QProgressBar::chunk {{
                background: {Colors.ACCENT_GREEN};
                border-radius: 2px;
            }}
        """)
        self.progress.hide()
        right_col.addWidget(self.progress)
        
        main_layout.addLayout(right_col, 65) # 65% width

    # ═══════════════════════════════════════════════════════════════════════════
    # LOGIC
    # ═══════════════════════════════════════════════════════════════════════════

    def _start_sync(self):
        self.stack.setCurrentIndex(0) # Loading
        self.spinner.start()
        
        self.sync_worker = IndexDownloadWorker()
        self.sync_worker.finished.connect(self._on_sync_done)
        self.sync_worker.progress.connect(self.status_label.setText)
        self.sync_worker.error.connect(self._on_sync_error)
        self.sync_worker.start()
    
    def _on_sync_done(self, app_ids):
        self.cached_app_ids = app_ids
        self.spinner.stop()
        self.stack.setCurrentIndex(1) # List
        self.status_label.setText(f"Online • {len(app_ids):,} supported games")
        self.search_input.setFocus()
    
    def _on_sync_error(self, err):
        self.spinner.stop()
        self.stack.setCurrentIndex(1)
        self.status_label.setText("Offline Mode")
        QMessageBox.warning(self, "Connection Error", f"Could not sync library:\n{err}")

    def on_search_change(self, text):
        self.debounce_timer.stop()
        if text.strip():
            self.debounce_timer.start(400)
        else:
            self.results_list.clear()
            
    def do_search(self):
        query = self.search_input.text().strip()
        if not query: return
        
        self.current_search_id += 1
        self.status_label.setText("Searching Store...")
        
        if self.active_reply: self.active_reply.abort()
        
        from PyQt6.QtCore import QUrlQuery
        url = QUrl("https://store.steampowered.com/api/storesearch")
        q = QUrlQuery()
        q.addQueryItem("term", query)
        q.addQueryItem("l", "english")
        q.addQueryItem("cc", "US")
        url.setQuery(q)
        
        req = QNetworkRequest(url)
        self.active_reply = self.network_manager.get(req)
        self.active_reply.setProperty("sid", self.current_search_id)

    def _on_search_finished(self, reply: QNetworkReply):
        reply.deleteLater()
        self.active_reply = None
        
        if reply.error() == QNetworkReply.NetworkError.OperationCanceledError: return
        if reply.property("sid") != self.current_search_id: return
        
        if reply.error() != QNetworkReply.NetworkError.NoError:
            self.status_label.setText("Search failed")
            return
            
        try:
            data = json.loads(reply.readAll().data().decode("utf-8"))
            self._display_results(data.get("items", []))
        except:
            self.status_label.setText("Invalid response")

    def _create_status_icon(self, supported: bool) -> QIcon:
        size = 64
        pixmap = QPixmap(size, size)
        pixmap.fill(Qt.GlobalColor.transparent)
        
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Color & Shape
        color = QColor(Colors.ACCENT_GREEN) if supported else QColor(Colors.ACCENT_RED)
        bg_color = QColor(color)
        bg_color.setAlpha(40) # Semi-transparent background
        
        # Draw Circle Background
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(bg_color))
        painter.drawEllipse(4, 4, size-8, size-8)
        
        # Draw Border
        painter.setPen(QPen(color, 3))
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawEllipse(4, 4, size-8, size-8)
        
        # Draw Symbol
        painter.setFont(QFont("Segoe UI Symbol", 28, QFont.Weight.Bold))
        painter.setPen(color)
        symbol = "✓" if supported else "✕"
        painter.drawText(pixmap.rect(), Qt.AlignmentFlag.AlignCenter, symbol)
        
        painter.end()
        return QIcon(pixmap)

    def _display_results(self, items):
        self.results_list.clear()
        self.selected_game = None
        self.btn_patch.setEnabled(False)
        
        if not items:
            self.status_label.setText("No results found")
            return
            
        for item in items:
            name = item.get("name", "Unknown")
            appid = str(item.get("id", ""))
            supported = appid in self.cached_app_ids
            
            status_text = "Supported" if supported else "Not Indexed"
            display_text = f"{name}\n{status_text} • ID: {appid}"
            
            list_item = QListWidgetItem(display_text)
            list_item.setData(Qt.ItemDataRole.UserRole, {"name": name, "appid": appid, "supported": supported})
            list_item.setIcon(self._create_status_icon(supported))
            
            # Custom styling for supported vs unsupported
            if supported:
                list_item.setForeground(QColor(Colors.ACCENT_GREEN))
            else:
                list_item.setForeground(QColor(Colors.TEXT_SECONDARY))
                
            self.results_list.addItem(list_item)
        
        self.status_label.setText(f"Found {len(items)} results")

    def on_game_selected(self, item):
        data = item.data(Qt.ItemDataRole.UserRole)
        if not data: return
        
        if data["supported"]:
            self.selected_game = data
            self.btn_patch.setEnabled(True)
            self.btn_patch.desc_text = f"Install patch for {data['name']}"
            self.btn_patch.update() # Repaint
            self.status_label.setText(f"Selected: {data['name']}")
        else:
            self.selected_game = None
            self.btn_patch.setEnabled(False)
            self.btn_patch.desc_text = "Patch unavailable for this game"
            self.btn_patch.update()
            self.status_label.setText("Game not supported")

    def do_patch(self):
        if not self.selected_game: return
        
        self.btn_patch.setEnabled(False)
        self.progress.setValue(0)
        self.progress.show()
        
        self.dl_worker = LuaDownloadWorker(self.selected_game["appid"])
        self.dl_worker.finished.connect(self._on_patch_done)
        self.dl_worker.progress.connect(lambda d,t: self.progress.setValue(int(d/t*100) if t>0 else 0))
        self.dl_worker.status.connect(self.status_label.setText)
        self.dl_worker.error.connect(self._on_patch_error)
        self.dl_worker.start()

    def _on_patch_done(self, path):
        try:
            dest = os.path.join(STEAM_PLUGIN_DIR, f"{self.selected_game['appid']}.lua")
            if not os.path.exists(STEAM_PLUGIN_DIR):
                os.makedirs(STEAM_PLUGIN_DIR)
            shutil.copy2(path, dest)
            
            self.progress.hide()
            self.btn_patch.setEnabled(True)
            self.status_label.setText("Patch Installed!")
            QMessageBox.information(self, "Success", f"Patch installed for {self.selected_game['name']}")
        except Exception as e:
            self._on_patch_error(str(e))

    def _on_patch_error(self, err):
        self.progress.hide()
        self.btn_patch.setEnabled(True)
        self.status_label.setText("Error")
        QMessageBox.critical(self, "Error", str(err))

    def do_restart(self):
        if QMessageBox.question(self, "Restart Steam?", "Close Steam and all games?") != QMessageBox.StandardButton.Yes:
            return
        
        self.restart_worker = RestartWorker()
        self.restart_worker.finished.connect(self.status_label.setText)
        self.restart_worker.start()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    app.setStyleSheet(STYLESHEET)
    
    # Set global font
    font = QFont("Segoe UI", 10)
    app.setFont(font)
    
    window = SteamPatcherApp()
    window.show()
    
    sys.exit(app.exec())
