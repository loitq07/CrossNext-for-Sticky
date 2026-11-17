
    /* Shared responsive navigation and touch controls. */
    html, body, .app-shell { min-height: 100vh; min-height: 100dvh; }
    .sidebar { overflow-y: auto; }
    .mobile-menu-btn { display: none; }
    .sidebar-backdrop { display: none; position: fixed; inset: 0; background: #0008; z-index: 90; }
    .sidebar-backdrop.show { display: block; }
    button:disabled { opacity: .5; cursor: not-allowed; }
    :focus-visible { outline: 2px solid hsl(var(--ring)); outline-offset: 3px; }
    .family-info, .setting-name, .device-val { overflow-wrap: anywhere; }
    .btn-small { display: inline-flex; align-items: center; justify-content: center; min-height: 36px; padding: .4rem .75rem; border: 1px solid hsl(var(--border)); border-radius: var(--radius); background: hsl(var(--secondary)); color: hsl(var(--foreground)); cursor: pointer; }
    .btn-delete { color: hsl(var(--destructive)); }
    .topbar-actions { display: flex; align-items: center; gap: .5rem; }
    /* Sidebar Footer (Device Info Card - shadcn style) */
    .sidebar-footer {
      margin-top: auto;
      background: hsl(var(--secondary) / 0.7);
      border: 1px solid hsl(var(--border));
      border-radius: var(--radius);
      padding: 0.75rem 0.85rem;
      box-shadow: 0 1px 2px rgba(0, 0, 0, 0.04);
      font-size: 0.8rem;
    }
    .footer-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 0.5rem;
      margin-bottom: 0.55rem;
      padding-bottom: 0.45rem;
      border-bottom: 1px solid hsl(var(--border));
    }
    .footer-status-pill {
      display: flex;
      align-items: center;
      gap: 0.4rem;
    }
    .status-indicator {
      width: 8px;
      height: 8px;
      background: #10b981;
      border-radius: 50%;
      box-shadow: 0 0 0 2px rgba(16, 185, 129, 0.25);
      display: inline-block;
      flex-shrink: 0;
      animation: status-pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
    }
    @keyframes status-pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.45; transform: scale(0.92); }
    }
    .footer-title {
      font-size: 0.7rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      color: hsl(var(--muted-foreground));
    }
    .badge-ver {
      font-size: 0.7rem;
      font-weight: 600;
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
      padding: 0.1rem 0.45rem;
      border-radius: 9999px;
      border: 1px solid hsl(var(--border));
      color: hsl(var(--muted-foreground));
      background: hsl(var(--card));
      cursor: pointer;
      transition: all 0.15s ease;
    }
    .badge-ver:hover {
      border-color: #10b981;
      color: #10b981;
    }
    .modal-backdrop {
      display: none;
      position: fixed;
      inset: 0;
      background: rgba(0, 0, 0, 0.65);
      backdrop-filter: blur(4px);
      z-index: 100;
      align-items: center;
      justify-content: center;
      padding: 1rem;
    }
    .modal-backdrop.show {
      display: flex;
    }
    .modal {
      background: hsl(var(--card));
      border: 1px solid hsl(var(--border));
      border-radius: calc(var(--radius) + 2px);
      box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.3);
      padding: 1.5rem;
    }
    .device-details {
      display: flex;
      flex-direction: column;
      gap: 0.35rem;
    }
    .detail-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.8rem;
      line-height: 1.4;
    }
    .detail-label {
      display: inline-flex;
      align-items: center;
      gap: 0.35rem;
      color: hsl(var(--muted-foreground));
    }
    .detail-label i {
      font-size: 0.95rem;
    }
    .detail-val {
      font-weight: 600;
      color: hsl(var(--foreground));
    }
    .detail-val.device-name {
      text-transform: capitalize;
    }
    .ip-row {
      cursor: pointer;
      border-radius: calc(var(--radius) - 2px);
      padding: 0.2rem 0.35rem;
      margin: -0.1rem -0.35rem;
      transition: background 0.15s ease, color 0.15s ease;
      user-select: none;
    }
    .ip-row:hover {
      background: hsl(var(--accent));
    }
    .ip-val {
      display: inline-flex;
      align-items: center;
      gap: 0.35rem;
      font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
      font-size: 0.775rem;
    }
    .copy-icon {
      font-size: 0.85rem;
      color: hsl(var(--muted-foreground));
      transition: all 0.15s ease;
    }
    .copy-icon.text-success {
      color: #10b981 !important;
    }
    .ip-row:hover .copy-icon {
      color: hsl(var(--foreground));
    }
    /* Desktop layout: Pinned Sidebar & Sticky Navbar */
    @media (min-width: 769px) {
      html, body {
        height: 100vh;
        height: 100dvh;
        overflow: hidden;
      }
      .app-shell {
        height: 100vh;
        height: 100dvh;
        display: flex;
        overflow: hidden;
        width: 100%;
      }
      .sidebar {
        height: 100vh;
        height: 100dvh;
        width: var(--sidebar-width);
        overflow-y: auto;
        overflow-x: hidden;
        display: flex;
        flex-direction: column;
        flex-shrink: 0;
        z-index: 30;
        padding: 1rem 0.85rem;
        gap: 1rem;
        scrollbar-width: thin;
      }
      .brand-box {
        flex-shrink: 0;
      }
      .nav-section {
        flex: 1;
        overflow-y: auto;
        min-height: 0;
        scrollbar-width: thin;
      }
      .sidebar-footer {
        margin-top: auto;
        flex-shrink: 0;
      }
      .main-wrapper {
        flex: 1;
        height: 100vh;
        height: 100dvh;
        display: flex;
        flex-direction: column;
        min-width: 0;
        overflow: hidden;
      }
      .topbar {
        position: sticky;
        top: 0;
        flex-shrink: 0;
        z-index: 25;
        background: hsl(var(--card) / 0.95);
        backdrop-filter: blur(10px);
        -webkit-backdrop-filter: blur(10px);
      }
      .content-container {
        flex: 1;
        display: flex;
        min-height: 0;
        overflow: hidden;
      }
      .content-main {
        flex: 1;
        overflow-y: auto;
        min-height: 0;
        min-width: 0;
        margin-bottom: 0;
        padding-bottom: 5rem;
      }
      .storage-panel {
        height: 100%;
        overflow-y: auto;
        flex-shrink: 0;
        z-index: 20;
      }
    }
    @media (max-width: 768px) {
      .sidebar { display: flex; position: fixed; inset: 0 auto 0 0; width: min(280px, 85vw); transform: translateX(-100%); z-index: 100; visibility: hidden; }
      .sidebar.mobile-open { transform: translateX(0); visibility: visible; }
      .sidebar-footer { margin-bottom: max(0.5rem, env(safe-area-inset-bottom)); }
      .mobile-menu-btn { display: inline-flex; align-items: center; justify-content: center; flex-shrink: 0; width: 44px; height: 44px; border: 1px solid hsl(var(--border)); border-radius: var(--radius); background: hsl(var(--card)); color: hsl(var(--foreground)); }
      body.nav-open { overflow: hidden; }
      .topbar { height: auto; min-height: 64px; padding: .6rem .75rem; flex-wrap: wrap; }
      .topbar-left { min-width: 0; flex: 1; }
      .breadcrumbs { min-width: 0; overflow-x: auto; max-width: 100%; }
      .content-main { padding: 1rem; }
      .btn, .btn-icon, .btn-small, .nav-item, .segmented-btn { min-height: 44px; }
      .btn-icon { min-width: 44px; }
      input:not([type=checkbox]):not([type=range]), select, textarea { font-size: 16px !important; max-width: 100%; }
      .setting-row { align-items: flex-start; gap: .65rem; }
      .setting-control { min-width: 0; flex-wrap: wrap; }
      .setting-control select, .setting-control input[type=text], .setting-control input[type=password] { width: 100%; min-width: 0; height: 44px; }
      .save-bar { left: 0; padding: .75rem 1rem calc(.75rem + env(safe-area-inset-bottom)); gap: .5rem; }
      .content-main { padding-bottom: calc(6rem + env(safe-area-inset-bottom)); }
      #topSaveBtn { display: none; }
      .upload-form { align-items: stretch; }
      .upload-form input[type=file] { min-width: 0; flex-basis: 100%; width: 100%; }
      .upload-form .btn { width: 100%; }
      .family { flex-wrap: wrap; gap: .5rem; }
      .family-info { flex-basis: 70%; }
      .card { padding: 1rem; }
      .modal-overlay { padding: .5rem; }
      .modal { width: 100%; max-height: calc(100dvh - 1rem); padding: 1.25rem; }
      .modal-actions, .convert-row, .advanced-setting-row { flex-wrap: wrap; gap: .5rem; }
      .modal-actions .btn { flex: 1; }
      #file-table { max-width: 100%; overflow-x: auto; }
      .file-table { min-width: 600px; }
      .file-actions { opacity: 1; }
      .recent-strip { grid-template-columns: repeat(2, minmax(0,1fr)); }
      .files-page .topbar > .breadcrumbs { flex: 1; }
      .files-page .topbar-actions { width: 100%; flex-wrap: wrap; }
      .files-page .search-box { width: 100%; flex-basis: 100%; }
      .files-page .topbar-actions > .btn { flex: 1; }
      #notification { left: 1rem; right: 1rem; bottom: 1rem; }
    }
