"""
Steam Lua Patcher - Cyberpunk Edition
A futuristic gaming-focused UI built with PyQt6
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from typing import Optional

from PyQt6.QtCore import (
    Qt, QThread, pyqtSignal, QTimer, QPropertyAnimation, 
    QEasingCurve, QPoint, QSize, pyqtProperty, QRect
)
from PyQt6.QtGui import (
    QFont, QFontDatabase, QColor, QPainter, QPen, QBrush,
    QLinearGradient, QRadialGradient, QPainterPath, QIcon,
    QPalette, QPixmap
)
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QTableWidget, QTableWidgetItem,
    QHeaderView, QMessageBox, QProgressBar, QFrame, QScrollArea,
    QGraphicsDropShadowEffect, QSizePolicy, QAbstractItemView
)




def get_resource_path(relative_path):
    """Get absolute path to resource, works for dev and for PyInstaller"""
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(base_path, relative_path)


# ═══════════════════════════════════════════════════════════════════════════════
# CYBERPUNK COLOR PALETTE
# ═══════════════════════════════════════════════════════════════════════════════

class CyberColors:
    # Backgrounds
    BG_DEEP = "#0a0a0f"
    BG_PANEL = "#13131a"
    BG_CARD = "#1a1a25"
    BG_HOVER = "#252535"
    
    # Neon Accents
    NEON_CYAN = "#00ffff"
    NEON_MAGENTA = "#ff00ff"
    NEON_YELLOW = "#ffff00"
    NEON_GREEN = "#00ff88"
    NEON_RED = "#ff3366"
    NEON_ORANGE = "#ff8800"
    NEON_BLUE = "#0088ff"
    
    # Text
    TEXT_PRIMARY = "#e0e0ff"
    TEXT_SECONDARY = "#a0a0cc"
    TEXT_MUTED = "#6666aa"
    
    # Borders
    BORDER_DARK = "#2a2a3a"
    BORDER_GLOW = "#00ffff40"


# ═══════════════════════════════════════════════════════════════════════════════
# GLOBAL STYLESHEET
# ═══════════════════════════════════════════════════════════════════════════════

CYBERPUNK_STYLESHEET = f"""
/* ══════════════ MAIN WINDOW ══════════════ */
QMainWindow {{
    background-color: {CyberColors.BG_DEEP};
}}

QWidget {{
    background-color: transparent;
    color: {CyberColors.TEXT_PRIMARY};
    font-family: 'Segoe UI', 'Consolas', monospace;
}}

/* ══════════════ LABELS ══════════════ */
QLabel {{
    color: {CyberColors.TEXT_PRIMARY};
    background: transparent;
}}

QLabel#title {{
    font-size: 32px;
    font-weight: bold;
    color: {CyberColors.NEON_CYAN};
}}

QLabel#subtitle {{
    font-size: 12px;
    color: {CyberColors.TEXT_MUTED};
    letter-spacing: 3px;
}}

/* ══════════════ LINE EDIT (SEARCH) ══════════════ */
QLineEdit {{
    background-color: {CyberColors.BG_PANEL};
    border: 2px solid {CyberColors.BORDER_DARK};
    border-radius: 8px;
    padding: 12px 16px;
    font-size: 14px;
    color: {CyberColors.TEXT_PRIMARY};
    selection-background-color: {CyberColors.NEON_CYAN};
    selection-color: {CyberColors.BG_DEEP};
}}

QLineEdit:focus {{
    border: 2px solid {CyberColors.NEON_CYAN};
    background-color: {CyberColors.BG_CARD};
}}

QLineEdit:hover {{
    border: 2px solid {CyberColors.NEON_CYAN}80;
}}

/* ══════════════ BUTTONS ══════════════ */
QPushButton {{
    background-color: {CyberColors.BG_PANEL};
    border: 2px solid {CyberColors.NEON_CYAN};
    border-radius: 8px;
    padding: 12px 24px;
    font-size: 13px;
    font-weight: bold;
    color: {CyberColors.NEON_CYAN};
    text-transform: uppercase;
    letter-spacing: 1px;
}}

QPushButton:hover {{
    background-color: {CyberColors.NEON_CYAN}20;
    border: 2px solid {CyberColors.NEON_CYAN};
    color: {CyberColors.NEON_CYAN};
}}

QPushButton:pressed {{
    background-color: {CyberColors.NEON_CYAN}40;
}}

QPushButton:disabled {{
    background-color: {CyberColors.BG_PANEL};
    border: 2px solid {CyberColors.BORDER_DARK};
    color: {CyberColors.TEXT_MUTED};
}}

QPushButton#patchBtn {{
    border-color: {CyberColors.NEON_GREEN};
    color: {CyberColors.NEON_GREEN};
}}

QPushButton#patchBtn:hover {{
    background-color: {CyberColors.NEON_GREEN}25;
    border-color: {CyberColors.NEON_GREEN};
}}

QPushButton#patchBtn:disabled {{
    border-color: {CyberColors.BORDER_DARK};
    color: {CyberColors.TEXT_MUTED};
}}

QPushButton#restartBtn {{
    border-color: {CyberColors.NEON_MAGENTA};
    color: {CyberColors.NEON_MAGENTA};
}}

QPushButton#restartBtn:hover {{
    background-color: transparent;
    border-color: #ff88ff;
    border-width: 3px;
}}

/* ══════════════ TABLE ══════════════ */
QTableWidget {{
    background-color: {CyberColors.BG_PANEL};
    border: 2px solid {CyberColors.BORDER_DARK};
    border-radius: 8px;
    gridline-color: {CyberColors.BORDER_DARK};
    outline: none;
}}

QTableWidget::item {{
    padding: 10px;
    border-bottom: 1px solid {CyberColors.BORDER_DARK};
}}

QTableWidget::item:selected {{
    background-color: {CyberColors.NEON_CYAN}30;
    color: {CyberColors.NEON_CYAN};
}}

QTableWidget::item:hover {{
    background-color: {CyberColors.BG_HOVER};
}}

QHeaderView::section {{
    background-color: {CyberColors.BG_CARD};
    color: {CyberColors.NEON_CYAN};
    padding: 12px;
    border: none;
    border-bottom: 2px solid {CyberColors.NEON_CYAN};
    font-weight: bold;
    text-transform: uppercase;
    letter-spacing: 1px;
}}

/* ══════════════ SCROLLBAR ══════════════ */
QScrollBar:vertical {{
    background-color: {CyberColors.BG_PANEL};
    width: 12px;
    border-radius: 6px;
    margin: 0;
}}

QScrollBar::handle:vertical {{
    background-color: {CyberColors.NEON_CYAN}60;
    border-radius: 6px;
    min-height: 30px;
}}

QScrollBar::handle:vertical:hover {{
    background-color: {CyberColors.NEON_CYAN};
}}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0;
}}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {{
    background: transparent;
}}

/* ══════════════ PROGRESS BAR ══════════════ */
QProgressBar {{
    background-color: {CyberColors.BG_PANEL};
    border: 2px solid {CyberColors.BORDER_DARK};
    border-radius: 8px;
    height: 20px;
    text-align: center;
    color: {CyberColors.TEXT_PRIMARY};
}}

QProgressBar::chunk {{
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 {CyberColors.NEON_CYAN},
        stop:0.5 {CyberColors.NEON_MAGENTA},
        stop:1 {CyberColors.NEON_CYAN});
    border-radius: 6px;
}}

/* ══════════════ FRAMES ══════════════ */
QFrame#cardFrame {{
    background-color: {CyberColors.BG_PANEL};
    border: 1px solid {CyberColors.BORDER_DARK};
    border-radius: 12px;
}}

QFrame#statusFrame {{
    background-color: {CyberColors.BG_CARD};
    border-top: 2px solid {CyberColors.NEON_CYAN}40;
}}

/* ══════════════ MESSAGE BOX ══════════════ */
QMessageBox {{
    background-color: {CyberColors.BG_PANEL};
}}

QMessageBox QLabel {{
    color: {CyberColors.TEXT_PRIMARY};
}}

QMessageBox QPushButton {{
    min-width: 80px;
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

from PyQt6.QtNetwork import QNetworkAccessManager, QNetworkRequest, QNetworkReply
from PyQt6.QtCore import QUrl

class InitWorker(QThread):
    """Thread for initializing the game data"""
    finished = pyqtSignal(str, set)  # Emits the lua_files_dir path and set of existing app IDs
    error = pyqtSignal(str)
    
    def run(self):
        try:
            lua_dir = ""
            existing_files = set()
            
            # Determine directory and extract if needed
            if getattr(sys, 'frozen', False):
                zip_path = get_resource_path("games_data.zip")
                temp_dir = os.path.join(tempfile.gettempdir(), "SteamLuaPatcher_Cache")
                
                # Only extract if valid directory doesn't exist
                should_extract = True
                if os.path.exists(temp_dir):
                    # Basic check: if it has files, assume it's good (optimization)
                    # In a real app we might check version/hash
                    if len(os.listdir(temp_dir)) > 0:
                        should_extract = False
                
                if should_extract and os.path.exists(zip_path):
                    if not os.path.exists(temp_dir):
                        os.makedirs(temp_dir)
                    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
                        zip_ref.extractall(temp_dir)
                
                lua_dir = temp_dir
            else:
                lua_dir = get_resource_path("All Games Files")
            
            # Index files for O(1) lookup
            if os.path.exists(lua_dir):
                for filename in os.listdir(lua_dir):
                    if filename.endswith(".lua"):
                        # Store just the ID part, assuming "12345.lua"
                        existing_files.add(filename[:-4])
            
            self.finished.emit(lua_dir, existing_files)
            
        except Exception as e:
            self.error.emit(str(e))



class RestartWorker(QThread):
    """Thread for restarting Steam"""
    finished = pyqtSignal(str)
    error = pyqtSignal(str)
    
    def run(self):
        try:
            subprocess.run("taskkill /F /IM steam.exe", shell=True, 
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(2)
            
            if os.path.exists(STEAM_EXE_PATH):
                subprocess.Popen([STEAM_EXE_PATH])
                self.finished.emit("Steam restarted successfully!")
            else:
                subprocess.run("start steam://open/main", shell=True)
                self.finished.emit("Steam restart command sent.")
        except Exception as e:
            self.error.emit(str(e))


# ═══════════════════════════════════════════════════════════════════════════════
# CUSTOM WIDGETS
# ═══════════════════════════════════════════════════════════════════════════════

class GlowingLabel(QLabel):
    """Label with neon glow effect"""
    def __init__(self, text: str, color: str = CyberColors.NEON_CYAN, parent=None):
        super().__init__(text, parent)
        self.glow_color = color
        self._setup_glow()
    
    def _setup_glow(self):
        glow = QGraphicsDropShadowEffect(self)
        glow.setBlurRadius(20)
        glow.setColor(QColor(self.glow_color))
        glow.setOffset(0, 0)
        self.setGraphicsEffect(glow)


class CyberButton(QPushButton):
    """Button with animated glow effect"""
    def __init__(self, text: str, color: str = CyberColors.NEON_CYAN, parent=None):
        super().__init__(text, parent)
        self.base_color = color
        self._glow_intensity = 15
        self._setup_glow()
    
    def _setup_glow(self):
        self.glow = QGraphicsDropShadowEffect(self)
        self.glow.setBlurRadius(self._glow_intensity)
        self.glow.setColor(QColor(self.base_color))
        self.glow.setOffset(0, 0)
        self.setGraphicsEffect(self.glow)
    
    def enterEvent(self, event):
        self.glow.setBlurRadius(30)
        super().enterEvent(event)
    
    def leaveEvent(self, event):
        self.glow.setBlurRadius(15)
        super().leaveEvent(event)


class NeonDivider(QFrame):
    """Horizontal divider with neon glow"""
    def __init__(self, color: str = CyberColors.NEON_CYAN, parent=None):
        super().__init__(parent)
        self.setFixedHeight(2)
        self.setStyleSheet(f"""
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 transparent,
                stop:0.2 {color},
                stop:0.8 {color},
                stop:1 transparent);
        """)
        
        glow = QGraphicsDropShadowEffect(self)
        glow.setBlurRadius(10)
        glow.setColor(QColor(color))
        glow.setOffset(0, 0)
        self.setGraphicsEffect(glow)


class PulsingProgress(QProgressBar):
    """Progress bar with pulsing animation for indeterminate state"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setTextVisible(False)
        self.setMinimum(0)
        self.setMaximum(0)  # Indeterminate


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN APPLICATION
# ═══════════════════════════════════════════════════════════════════════════════

class SteamPatcherApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("⚡ STEAM LUA PATCHER // CYBERPUNK EDITION")
        self.setFixedSize(600, 700)
        
        # Set window icon
        try:
            self.setWindowIcon(QIcon(get_resource_path("logo.ico")))
        except:
            pass
        
        # Data
        self.lua_files_dir: Optional[str] = None
        self.cached_files: set = set()
        self.search_results = []
        self.current_search_id = 0
        self.debounce_timer = QTimer()
        self.debounce_timer.setSingleShot(True)
        self.debounce_timer.timeout.connect(self.start_search)
        
        # Network Manager
        self.network_manager = QNetworkAccessManager()
        self.network_manager.finished.connect(self._on_search_finished)
        self.active_reply = None
        
        # Setup UI
        self._setup_ui()
        
        # Start initialization
        self._start_initialization()
    
    def _setup_ui(self):
        """Create all UI components"""
        # Central widget
        central = QWidget()
        self.setCentralWidget(central)
        
        # Main layout
        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(30, 30, 30, 20)
        main_layout.setSpacing(20)
        
        # ═══════════ HEADER ═══════════
        header_layout = QVBoxLayout()
        header_layout.setSpacing(5)
        
        # Title with glow
        self.title_label = GlowingLabel("STEAM LUA PATCHER", CyberColors.NEON_CYAN)
        self.title_label.setObjectName("title")
        self.title_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header_layout.addWidget(self.title_label)
        
        # Subtitle
        subtitle = QLabel("// CYBERPUNK EDITION v2.0")
        subtitle.setObjectName("subtitle")
        subtitle.setAlignment(Qt.AlignmentFlag.AlignCenter)
        header_layout.addWidget(subtitle)
        
        # Neon divider
        header_layout.addWidget(NeonDivider())
        
        main_layout.addLayout(header_layout)
        
        # ═══════════ SEARCH SECTION ═══════════
        search_frame = QFrame()
        search_frame.setObjectName("cardFrame")
        search_layout = QVBoxLayout(search_frame)
        search_layout.setContentsMargins(20, 15, 20, 15)
        
        search_header = QLabel("🔍 GAME SEARCH")
        search_header.setStyleSheet(f"""
            font-size: 12px;
            font-weight: bold;
            color: {CyberColors.NEON_CYAN};
            letter-spacing: 2px;
            margin-bottom: 5px;
        """)
        search_layout.addWidget(search_header)
        
        self.search_input = QLineEdit()
        self.search_input.setPlaceholderText("Enter game name to search Steam database...")
        self.search_input.textChanged.connect(self.on_search_change)
        self.search_input.setEnabled(False)
        search_layout.addWidget(self.search_input)
        
        main_layout.addWidget(search_frame)
        
        # ═══════════ RESULTS TABLE ═══════════
        results_frame = QFrame()
        results_frame.setObjectName("cardFrame")
        results_layout = QVBoxLayout(results_frame)
        results_layout.setContentsMargins(20, 15, 20, 15)
        
        results_header = QLabel("📋 SEARCH RESULTS")
        results_header.setStyleSheet(f"""
            font-size: 12px;
            font-weight: bold;
            color: {CyberColors.NEON_CYAN};
            letter-spacing: 2px;
            margin-bottom: 5px;
        """)
        results_layout.addWidget(results_header)
        
        self.table = QTableWidget()
        self.table.setColumnCount(3)
        self.table.setHorizontalHeaderLabels(["GAME NAME", "APP ID", "STATUS"])
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(1, QHeaderView.ResizeMode.Fixed)
        self.table.horizontalHeader().setSectionResizeMode(2, QHeaderView.ResizeMode.Fixed)
        self.table.setColumnWidth(1, 80)
        self.table.setColumnWidth(2, 120)
        self.table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        self.table.verticalHeader().setVisible(False)
        self.table.setShowGrid(False)
        self.table.setWordWrap(True)  # Enable word wrap for long names
        self.table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
        self.table.itemSelectionChanged.connect(self.on_selection_change)
        results_layout.addWidget(self.table)
        
        main_layout.addWidget(results_frame, 1)  # stretch
        
        # ═══════════ ACTION BUTTONS ═══════════
        actions_layout = QHBoxLayout()
        actions_layout.setSpacing(15)
        
        self.patch_btn = CyberButton("⚡ PATCH SELECTED GAME", CyberColors.NEON_GREEN)
        self.patch_btn.setObjectName("patchBtn")
        self.patch_btn.setEnabled(False)
        self.patch_btn.clicked.connect(self.patch_selected)
        self.patch_btn.setMinimumHeight(50)
        actions_layout.addWidget(self.patch_btn)
        
        self.restart_btn = CyberButton("🔄 RESTART STEAM", CyberColors.NEON_MAGENTA)
        self.restart_btn.setObjectName("restartBtn")
        self.restart_btn.clicked.connect(self.restart_steam)
        self.restart_btn.setMinimumHeight(50)
        actions_layout.addWidget(self.restart_btn)
        
        main_layout.addLayout(actions_layout)
        
        # ═══════════ STATUS BAR ═══════════
        status_frame = QFrame()
        status_frame.setObjectName("statusFrame")
        status_frame.setFixedHeight(40)
        status_layout = QHBoxLayout(status_frame)
        status_layout.setContentsMargins(15, 5, 15, 5)
        
        self.status_indicator = QLabel("●")
        self.status_indicator.setStyleSheet(f"color: {CyberColors.NEON_CYAN}; font-size: 10px;")
        status_layout.addWidget(self.status_indicator)
        
        self.status_label = QLabel("INITIALIZING SYSTEM...")
        self.status_label.setStyleSheet(f"""
            font-family: 'Consolas', monospace;
            font-size: 12px;
            color: {CyberColors.TEXT_SECONDARY};
            letter-spacing: 1px;
        """)
        status_layout.addWidget(self.status_label)
        status_layout.addStretch()
        
        main_layout.addWidget(status_frame)
        
        # Progress bar (hidden initially)
        self.progress = PulsingProgress()
        self.progress.setFixedHeight(4)
        self.progress.hide()
        main_layout.addWidget(self.progress)
    
    def _start_initialization(self):
        """Start loading game data in background"""
        self.progress.show()
        self.set_status("LOADING DATABASE...", CyberColors.NEON_YELLOW)
        
        self.init_worker = InitWorker()
        self.init_worker.finished.connect(self._on_init_complete)
        self.init_worker.error.connect(self._on_init_error)
        self.init_worker.start()
    
    def _on_init_complete(self, lua_dir: str, existing_files: set):
        """Called when initialization is complete"""
        self.lua_files_dir = lua_dir
        self.cached_files = existing_files
        self.progress.hide()
        self.search_input.setEnabled(True)
        self.search_input.setFocus()
        self.set_status("SYSTEM READY // ENTER GAME NAME TO SEARCH", CyberColors.NEON_GREEN)
    
    def _on_init_error(self, error: str):
        """Called when initialization fails"""
        self.progress.hide()
        self.set_status(f"INIT ERROR: {error}", CyberColors.NEON_RED)
        QMessageBox.critical(self, "Initialization Error", f"Failed to load game data:\n{error}")
    
    def set_status(self, message: str, color: str = CyberColors.NEON_CYAN):
        """Update status bar"""
        self.status_label.setText(message)
        self.status_indicator.setStyleSheet(f"color: {color}; font-size: 10px;")
    
    def on_search_change(self, text: str):
        """Handle search input changes with debouncing"""
        self.debounce_timer.stop()
        if text.strip():
            self.debounce_timer.start(400)  # 400ms debounce
    
    def start_search(self):
        """Execute the search"""
        query = self.search_input.text().strip()
        if not query:
            return
        
        self.current_search_id += 1
        
        self.set_status(f"SEARCHING: '{query.upper()}'...", CyberColors.NEON_YELLOW)
        
        # Cancel previous request if any
        if self.active_reply:
            self.active_reply.abort()
            self.active_reply = None
            
        url = QUrl("https://store.steampowered.com/api/storesearch")
        query_items = [
            ("term", query),
            ("l", "english"),
            ("cc", "US")
        ]
        
        from PyQt6.QtCore import QUrlQuery
        q = QUrlQuery()
        for k, v in query_items:
            q.addQueryItem(k, v)
        url.setQuery(q)
        
        request = QNetworkRequest(url)
        self.active_reply = self.network_manager.get(request)
        self.active_reply.setProperty("search_id", self.current_search_id)
    
    def _on_search_finished(self, reply: QNetworkReply):
        """Handle search results"""
        reply.deleteLater()
        self.active_reply = None
        
        if reply.error() == QNetworkReply.NetworkError.OperationCanceledError:
            return  # Ignore cancelled requests
            
        search_id = reply.property("search_id")
        if search_id != self.current_search_id:
            return  # Stale result
            
        if reply.error() != QNetworkReply.NetworkError.NoError:
            self.set_status(f"NETWORK ERROR: {reply.errorString()}", CyberColors.NEON_RED)
            return

        try:
            import json
            data = json.loads(reply.readAll().data().decode("utf-8"))
            items = data.get("items", [])
            self._update_results_table(items)
            
        except Exception as e:
            self.set_status(f"PARSE ERROR: {str(e)}", CyberColors.NEON_RED)
            
    def _update_results_table(self, items: list):
        self.table.setUpdatesEnabled(False)
        self.table.setSortingEnabled(False)
        self.table.setRowCount(0)
        
        self.search_results = items
        
        if not items:
            self.set_status("NO RESULTS FOUND", CyberColors.NEON_ORANGE)
            self.table.setUpdatesEnabled(True)
            self.table.setSortingEnabled(True)
            return
        
        for item in items:
            name = item.get("name", "Unknown")
            appid = str(item.get("id", ""))
            
            # Use cached Set for O(1) lookup
            exists = appid in self.cached_files
            
            status = "✓ AVAILABLE" if exists else "✗ MISSING"
            status_color = CyberColors.NEON_GREEN if exists else CyberColors.NEON_RED
            
            row = self.table.rowCount()
            self.table.insertRow(row)
            
            # Name
            name_item = QTableWidgetItem(name)
            name_item.setFlags(name_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            name_item.setTextAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter | Qt.TextFlag.TextWordWrap)
            name_item.setToolTip(name)
            self.table.setItem(row, 0, name_item)
            
            # App ID
            appid_item = QTableWidgetItem(appid)
            appid_item.setFlags(appid_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            appid_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            self.table.setItem(row, 1, appid_item)
            
            # Status
            status_item = QTableWidgetItem(status)
            status_item.setFlags(status_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            status_item.setTextAlignment(Qt.AlignmentFlag.AlignCenter)
            status_item.setForeground(QColor(status_color))
            self.table.setItem(row, 2, status_item)
        
        self.table.resizeRowsToContents()
        self.table.setUpdatesEnabled(True)
        self.table.setSortingEnabled(True)
        self.set_status(f"FOUND {len(items)} RESULTS", CyberColors.NEON_GREEN)

    
    def on_selection_change(self):
        """Handle table selection changes"""
        selected = self.table.selectedItems()
        if selected:
            row = selected[0].row()
            status_item = self.table.item(row, 2)
            name_item = self.table.item(row, 0)
            
            if status_item and "AVAILABLE" in status_item.text():
                self.patch_btn.setEnabled(True)
                self.set_status(f"SELECTED: {name_item.text().upper()}", CyberColors.NEON_CYAN)
            else:
                self.patch_btn.setEnabled(False)
                self.set_status(f"LUA PATCH MISSING FOR: {name_item.text().upper()}", CyberColors.NEON_ORANGE)
        else:
            self.patch_btn.setEnabled(False)
    
    def patch_selected(self):
        """Copy the lua file to Steam plugin directory"""
        selected = self.table.selectedItems()
        if not selected:
            return
        
        row = selected[0].row()
        name = self.table.item(row, 0).text()
        appid = self.table.item(row, 1).text()
        
        if not self.lua_files_dir:
            return
        
        src_file = os.path.join(self.lua_files_dir, f"{appid}.lua")
        dest_file = os.path.join(STEAM_PLUGIN_DIR, f"{appid}.lua")
        
        try:
            if not os.path.exists(STEAM_PLUGIN_DIR):
                os.makedirs(STEAM_PLUGIN_DIR)
            
            shutil.copy2(src_file, dest_file)
            
            QMessageBox.information(
                self, 
                "⚡ PATCH SUCCESSFUL",
                f"Successfully patched:\n\n🎮 {name}\n📁 App ID: {appid}"
            )
            self.set_status(f"PATCHED: {name.upper()}", CyberColors.NEON_GREEN)
            
        except Exception as e:
            QMessageBox.critical(
                self,
                "❌ PATCH FAILED",
                f"Failed to copy lua file:\n\n{str(e)}"
            )
            self.set_status(f"PATCH FAILED: {str(e)}", CyberColors.NEON_RED)
    
    def restart_steam(self):
        """Restart Steam application"""
        reply = QMessageBox.question(
            self,
            "🔄 CONFIRM RESTART",
            "This will close Steam and all running games.\n\nContinue?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No
        )
        
        if reply != QMessageBox.StandardButton.Yes:
            return
        
        self.set_status("RESTARTING STEAM...", CyberColors.NEON_MAGENTA)
        
        self.restart_worker = RestartWorker()
        self.restart_worker.finished.connect(
            lambda msg: self.set_status(msg, CyberColors.NEON_GREEN)
        )
        self.restart_worker.error.connect(
            lambda err: (
                self.set_status(f"ERROR: {err}", CyberColors.NEON_RED),
                QMessageBox.critical(self, "Error", f"Failed to restart Steam:\n{err}")
            )
        )
        self.restart_worker.start()


# ═══════════════════════════════════════════════════════════════════════════════
# ENTRY POINT
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    # High DPI support
    if hasattr(Qt, 'AA_EnableHighDpiScaling'):
        QApplication.setAttribute(Qt.ApplicationAttribute.AA_EnableHighDpiScaling, True)
    if hasattr(Qt, 'AA_UseHighDpiPixmaps'):
        QApplication.setAttribute(Qt.ApplicationAttribute.AA_UseHighDpiPixmaps, True)
    
    app = QApplication(sys.argv)
    app.setStyle('Fusion')  # Consistent cross-platform look
    app.setStyleSheet(CYBERPUNK_STYLESHEET)
    
    # Set application font
    font = QFont("Segoe UI", 10)
    app.setFont(font)
    
    window = SteamPatcherApp()
    window.show()
    
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
