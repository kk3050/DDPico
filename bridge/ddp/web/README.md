# DDPico Bridge Web Dashboard

Modern, feature-rich web dashboard for monitoring the DDP Serial Bridge in real-time.

## Features

### 📊 Real-time Monitoring
- Live packet statistics (RX/TX)
- Data throughput visualization
- Interactive packet rate chart
- Connection status indicator
- System uptime counter

### 💻 Enhanced Console
- Color-coded log messages
- Filterable log types (Error, Info, Debug)
- Pause/Resume functionality
- Auto-scroll toggle
- Export logs to file
- Configurable line limits

### ⚙️ Settings
- Theme selection (Dark/Light/Auto)
- Update interval configuration
- Console line limit adjustment
- Sound notifications toggle
- Timestamp display toggle
- Settings persist in localStorage

### ⌨️ Keyboard Shortcuts
- `Ctrl/Cmd + K` - Clear console
- `Ctrl/Cmd + P` - Pause/Resume
- `Escape` - Close modal

## File Structure

```
web/
├── index.html          # Main HTML structure
├── css/
│   └── style.css      # Styles and responsive design
└── js/
    ├── chart.js       # Simple chart library for packet visualization
    └── app.js         # Main application logic
```

## Usage

Run the bridge with the new version:

```bash
python ddp_serial_bridge_v2.py
```

The dashboard will automatically open at `http://localhost:4000`

## Browser Compatibility

- Chrome/Edge (recommended)
- Firefox
- Safari
- Opera

## Technical Details

### Communication
- Server-Sent Events (SSE) for real-time updates
- JSON data format
- 100ms update interval (configurable)

### Chart
- Custom canvas-based chart implementation
- 60 data points (1 minute at 1 update/sec)
- Automatic scaling
- Dual-line visualization (RX/TX)

### Performance
- Efficient DOM updates
- Automatic log pruning
- Minimal memory footprint
- Smooth animations with CSS transitions

## Customization

Edit the CSS variables in `style.css` to customize colors:

```css
:root {
    --primary-gradient: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    --success: #10b981;
    --error: #ef4444;
    --warning: #f59e0b;
    --info: #3b82f6;
}
```

## Development

To modify the dashboard:

1. Edit files in the `web/` directory
2. Refresh browser to see changes
3. No build process required

## License

Part of the DDPico project.
