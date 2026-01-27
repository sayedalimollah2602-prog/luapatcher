"""
Steam Lua Patcher - Webserver
Flask application to serve Lua files for the Steam Lua Patcher desktop app.
"""

from flask import Flask, send_from_directory, jsonify, abort, request, Response
import os
from functools import wraps
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

app = Flask(__name__)

# Security configuration
# These should be set as environment variables in Vercel/Production or in a .env file locally
ACCESS_TOKEN = os.environ.get('SERVER_ACCESS_TOKEN')
ADMIN_PASSWORD = os.environ.get('ADMIN_PASSWORD')

if not ACCESS_TOKEN or not ADMIN_PASSWORD:
    print("WARNING: SERVER_ACCESS_TOKEN or ADMIN_PASSWORD not set in environment!")

# Directory containing all Lua game files
GAMES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'games')

def require_token(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        token = request.headers.get('X-Access-Token')
        if not token or token != ACCESS_TOKEN:
            return jsonify({'error': 'Unauthorized', 'message': 'Invalid or missing access token'}), 401
        return f(*args, **kwargs)
    return decorated_function

def check_auth(username, password):
    """Check if a username password combination is valid."""
    return username == 'admin' and password == ADMIN_PASSWORD

def authenticate():
    """Sends a 401 response that enables basic auth"""
    return Response(
    'Could not verify your access level for that URL.\n'
    'You have to login with proper credentials', 401,
    {'WWW-Authenticate': 'Basic realm="Login Required"'})

@app.route('/')
def index():
    """Health check endpoint"""
    return jsonify({
        'status': 'ok',
        'service': 'Steam Lua Patcher API',
        'version': '1.1.0',
        'security': 'enabled'
    })

@app.route('/admin')
def admin_panel():
    """Secure admin panel to view the access token"""
    auth = request.authorization
    if not auth or not check_auth(auth.username, auth.password):
        return authenticate()
    
    return f"""
    <html>
        <head>
            <title>Admin Panel - Steam Lua Patcher</title>
            <style>
                body {{ font-family: sans-serif; padding: 50px; background: #121212; color: #eee; }}
                .container {{ max-width: 600px; margin: 0 auto; background: #1e1e1e; padding: 30px; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }}
                h1 {{ color: #bb86fc; }}
                .token-box {{ background: #2c2c2c; padding: 15px; border-radius: 4px; border: 1px solid #333; font-family: monospace; font-size: 1.2em; word-break: break-all; margin-top: 20px; }}
            </style>
        </head>
        <body>
            <div class="container">
                <h1>Admin Panel</h1>
                <p>Use the following token in your GitHub Secrets (SERVER_ACCESS_TOKEN) and during app compilation.</p>
                <div class="token-box">{ACCESS_TOKEN}</div>
            </div>
        </body>
    </html>
    """

@app.route('/lua/<filename>')
@require_token
def serve_lua(filename):
    """Serve a specific Lua file by filename (e.g., 730.lua)"""
    if not filename.endswith('.lua'):
        filename = f"{filename}.lua"
    
    file_path = os.path.join(GAMES_DIR, filename)
    if not os.path.exists(file_path):
        abort(404, description=f"Lua file '{filename}' not found")
    
    return send_from_directory(GAMES_DIR, filename, mimetype='text/plain')


@app.route('/api/games_index.json')
@require_token
def serve_index():
    """Serve the games index JSON file"""
    base_dir = os.path.dirname(os.path.abspath(__file__))
    index_path = os.path.join(base_dir, 'games_index.json')
    
    if os.path.exists(index_path):
        return send_from_directory(base_dir, 'games_index.json', mimetype='application/json')
    
    abort(404, description="games_index.json not found. Run generate_index.py first.")


@app.route('/api/check/<app_id>')
@require_token
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
