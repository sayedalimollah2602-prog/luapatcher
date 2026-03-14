const express = require('express');
const serverless = require('serverless-http');
const path = require('path');
const fs = require('fs');
const auth = require('basic-auth');

// Note: Netlify injects environment variables from the dashboard in production.
// This is for local development if run directly with `netlify dev` or node.
require('dotenv').config({ path: path.join(__dirname, '../../.env') });

const app = express();

const ACCESS_TOKEN = process.env.SERVER_ACCESS_TOKEN;
const ADMIN_PASSWORD = process.env.ADMIN_PASSWORD;

if (!ACCESS_TOKEN || !ADMIN_PASSWORD) {
    console.warn("WARNING: SERVER_ACCESS_TOKEN or ADMIN_PASSWORD not set in environment!");
}

const GAMES_DIR = path.join(__dirname, '../../games');
const FIX_FILES_DIR = path.join(__dirname, '../../game-fix-files');
const INDEX_JSON = path.join(__dirname, '../../games_index.json');

// Middleware to check access token
const requireToken = (req, res, next) => {
    const token = req.get('X-Access-Token');
    if (!token || token !== ACCESS_TOKEN) {
        return res.status(401).json({ error: 'Unauthorized', message: 'Invalid or missing access token' });
    }
    next();
};

app.get('/', (req, res) => {
    res.json({
        status: 'ok',
        service: 'Steam Lua Patcher API',
        version: '1.1.0',
        security: 'enabled'
    });
});

app.get('/admin', (req, res) => {
    const credentials = auth(req);
    if (!credentials || credentials.name !== 'admin' || credentials.pass !== ADMIN_PASSWORD) {
        res.setHeader('WWW-Authenticate', 'Basic realm="Login Required"');
        return res.status(401).send('Could not verify your access level for that URL.\nYou have to login with proper credentials');
    }

    let gamesListHtml = "";
    try {
        if (fs.existsSync(INDEX_JSON)) {
            const data = JSON.parse(fs.readFileSync(INDEX_JSON, 'utf8'));
            const games = data.games || [];
            
            const rows = games.map(game => {
                const gameId = game.id || 'N/A';
                const gameName = game.name || 'N/A';
                const hasFix = game.has_fix ? '✓' : '';
                return `<tr><td class='id-col'>${gameId}</td><td class='name-col'>${gameName}</td><td class='fix-col'>${hasFix}</td></tr>`;
            });
            gamesListHtml = rows.join('\n');
        } else {
            gamesListHtml = "<tr><td colspan='3'>games_index.json not found</td></tr>";
        }
    } catch (e) {
        gamesListHtml = `<tr><td colspan='3'>Error loading games: ${e.message}</td></tr>`;
    }

    const html = `
    <html>
        <head>
            <title>Admin Panel - Steam Lua Patcher</title>
            <style>
                body { font-family: 'Segoe UI', sans-serif; padding: 50px; background: #121212; color: #eee; }
                .container { max-width: 900px; margin: 0 auto; background: #1e1e1e; padding: 30px; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
                h1 { color: #bb86fc; margin-bottom: 20px; }
                h2 { color: #03dac6; margin-top: 30px; margin-bottom: 10px; }
                .token-box { background: #2c2c2c; padding: 15px; border-radius: 4px; border: 1px solid #333; font-family: monospace; font-size: 1.2em; word-break: break-all; margin-bottom: 20px; }
                
                /* Search Bar */
                #searchInput {
                    width: 100%;
                    padding: 12px;
                    margin-bottom: 20px;
                    background: #2c2c2c;
                    border: 1px solid #444;
                    color: #fff;
                    border-radius: 4px;
                    font-size: 16px;
                }
                #searchInput:focus { outline: none; border-color: #bb86fc; }
                
                /* Table */
                table { width: 100%; border-collapse: collapse; margin-top: 10px; }
                th, td { padding: 12px; text-align: left; border-bottom: 1px solid #333; }
                th { background-color: #2c2c2c; color: #bb86fc; }
                tr:hover { background-color: #2a2a2a; }
                .id-col { width: 150px; font-family: monospace; color: #03dac6; }
                .fix-col { width: 60px; text-align: center; color: #bb86fc; }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>Admin Panel</h1>
                <p>Use the following token in your GitHub Secrets (SERVER_ACCESS_TOKEN) and during app compilation.</p>
                <div class="token-box">${ACCESS_TOKEN}</div>
                
                <h2>Available Games</h2>
                <input type="text" id="searchInput" onkeyup="searchGames()" placeholder="Search for games by name or ID...">
                
                <table id="gamesTable">
                    <thead>
                        <tr>
                            <th>ID</th>
                            <th>Name</th>
                            <th>Fix</th>
                        </tr>
                    </thead>
                    <tbody>
                        ${gamesListHtml}
                    </tbody>
                </table>
            </div>
            
            <script>
                function searchGames() {
                    var input, filter, table, tr, tdId, tdName, i, txtValueId, txtValueName;
                    input = document.getElementById("searchInput");
                    filter = input.value.toUpperCase();
                    table = document.getElementById("gamesTable");
                    tr = table.getElementsByTagName("tr");
                    
                    for (i = 0; i < tr.length; i++) {
                        tdId = tr[i].getElementsByTagName("td")[0];
                        tdName = tr[i].getElementsByTagName("td")[1];
                        
                        if (tdId || tdName) {
                            txtValueId = tdId ? (tdId.textContent || tdId.innerText) : "";
                            txtValueName = tdName ? (tdName.textContent || tdName.innerText) : "";
                            
                            if (txtValueId.toUpperCase().indexOf(filter) > -1 || txtValueName.toUpperCase().indexOf(filter) > -1) {
                                tr[i].style.display = "";
                            } else {
                                tr[i].style.display = "none";
                            }
                        }
                    }
                }
            </script>
        </body>
    </html>
    `;
    res.send(html);
});

app.get('/lua/:filename', requireToken, (req, res) => {
    let filename = req.params.filename;
    if (!filename.endsWith('.lua')) {
        filename = `${filename}.lua`;
    }
    
    const filePath = path.join(GAMES_DIR, filename);
    if (!fs.existsSync(filePath)) {
        return res.status(404).send(`Lua file '${filename}' not found`);
    }
    
    res.sendFile(filePath, {
        headers: {
            'Content-Type': 'text/plain'
        }
    });
});

app.get('/fix/:filename', requireToken, (req, res) => {
    let filename = req.params.filename;
    if (!filename.endsWith('.zip')) {
        filename = `${filename}.zip`;
    }
    
    const filePath = path.join(FIX_FILES_DIR, filename);
    if (!fs.existsSync(filePath)) {
        return res.status(404).send(`Fix file '${filename}' not found`);
    }
    
    res.sendFile(filePath, {
        headers: {
            'Content-Type': 'application/zip'
        }
    });
});

app.get('/api/games_index.json', requireToken, (req, res) => {
    if (fs.existsSync(INDEX_JSON)) {
        return res.sendFile(INDEX_JSON, {
            headers: {
                'Content-Type': 'application/json'
            }
        });
    }
    res.status(404).send("games_index.json not found. Run generate_index.py first.");
});

app.get('/api/check/:app_id', requireToken, (req, res) => {
    const appId = req.params.app_id;
    const filePath = path.join(GAMES_DIR, `${appId}.lua`);
    const exists = fs.existsSync(filePath);
    res.json({
        app_id: appId,
        available: exists
    });
});

module.exports.handler = serverless(app);
