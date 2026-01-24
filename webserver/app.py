"""
Steam Lua Patcher - Webserver
Flask application to serve Lua files for the Steam Lua Patcher desktop app.
"""

from flask import Flask, send_from_directory, jsonify, abort
import os

app = Flask(__name__)

# Directory containing all Lua game files
GAMES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')


@app.route('/')
def index():
    """Health check endpoint"""
    return jsonify({
        'status': 'ok',
        'service': 'Steam Lua Patcher API',
        'version': '1.0.0'
    })


@app.route('/lua/<filename>')
def serve_lua(filename):
    """Serve a specific Lua file by filename (e.g., 730.lua)"""
    if not filename.endswith('.lua'):
        filename = f"{filename}.lua"
    
    file_path = os.path.join(GAMES_DIR, filename)
    if not os.path.exists(file_path):
        abort(404, description=f"Lua file '{filename}' not found")
    
    return send_from_directory(GAMES_DIR, filename, mimetype='text/plain')


@app.route('/api/games_index.json')
def serve_index():
    """Serve the games index JSON file"""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    index_path = os.path.join(base_dir, 'games_index.json')
    
    if os.path.exists(index_path):
        return send_from_directory(base_dir, 'games_index.json', mimetype='application/json')
    
    abort(404, description="games_index.json not found. Run generate_index.py first.")


@app.route('/api/check/<app_id>')
def check_availability(app_id):
    """Check if a specific app ID has a Lua file available"""
    file_path = os.path.join(GAMES_DIR, f"{app_id}.lua")
    exists = os.path.exists(file_path)
    return jsonify({
        'app_id': app_id,
        'available': exists
    })


if __name__ == '__main__':
    # Local development server
    print(f"Games directory: {GAMES_DIR}")
    print(f"Files available: {len(os.listdir(GAMES_DIR)) if os.path.exists(GAMES_DIR) else 0}")
    app.run(debug=True, port=5000)
