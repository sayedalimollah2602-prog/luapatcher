# Steam Lua Patcher - Webserver

Flask-based file server for Steam Lua patches. This serves as the backend for the Steam Lua Patcher desktop application.

## Deployment

### Vercel Deployment

1. Fork this repository
2. Connect to Vercel
3. Deploy automatically

### Local Development

```bash
pip install -r requirements.txt
python app.py
```

Server runs at `http://localhost:5000`

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /` | Health check |
| `GET /lua/<app_id>.lua` | Get Lua file for specific app ID |
| `GET /api/games_index.json` | Get JSON index of all available app IDs |
| `GET /api/check/<app_id>` | Check if app ID has Lua file available |

## File Structure

```
webserver/
├── app.py              # Flask application
├── games/              # Lua files directory
│   ├── 730.lua
│   ├── 570.lua
│   └── ...
├── games_index.json    # Auto-generated index
├── generate_index.py   # Index generator script
├── requirements.txt    # Python dependencies
├── vercel.json         # Vercel config
└── .github/
    └── workflows/
        └── generate-index.yml
```

## GitHub Actions

The `generate-index.yml` workflow automatically:
1. Scans all `.lua` files in `games/`
2. Generates `games_index.json`
3. Creates a GitHub Release with the JSON file
4. Commits the updated index

This runs on every push to `main` that modifies files in `games/`.
