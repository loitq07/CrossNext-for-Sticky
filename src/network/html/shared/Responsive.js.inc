
  // Pages with a language switch define cnLang(); the others are Vietnamese-only.
  function cnText(en, vi) {
    return typeof cnLang === 'function' && cnLang() !== 'vi' ? en : vi;
  }

  function toggleMobileSidebar(force) {
    const sidebar = document.getElementById('sidebar');
    const open = typeof force === 'boolean' ? force : !sidebar.classList.contains('mobile-open');
    sidebar.classList.toggle('mobile-open', open);
    document.getElementById('sidebarBackdrop').classList.toggle('show', open);
    document.body.classList.toggle('nav-open', open);
    const button = document.getElementById('mobileMenuBtn');
    button.setAttribute('aria-expanded', String(open));
    document.querySelector('.main-wrapper').inert = open;
    if (open) sidebar.querySelector('a, button').focus(); else button.focus();
  }
  document.addEventListener('keydown', function(e) {
    const sidebar = document.getElementById('sidebar');
    if (!sidebar.classList.contains('mobile-open')) return;
    if (e.key === 'Escape') { toggleMobileSidebar(false); return; }
    if (e.key === 'Tab') {
      const items = [...sidebar.querySelectorAll('a, button, [tabindex="0"]')];
      const first = items[0], last = items[items.length - 1];
      if (e.shiftKey && document.activeElement === first) { e.preventDefault(); last.focus(); }
      if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus(); }
    }
  });
  window.matchMedia('(max-width: 768px)').addEventListener('change', function(e) {
    if (!e.matches) toggleMobileSidebar(false);
  });

  function copyDeviceIp() {
    const ipEl = document.getElementById('sbIp');
    const ip = ipEl ? ipEl.textContent.trim() : window.location.hostname;
    if (!ip) return;
    const copyBtn = document.querySelector('.ip-row');
    const copyIcon = copyBtn ? copyBtn.querySelector('.copy-icon') : null;
    const doFeedback = () => {
      if (copyIcon) {
        copyIcon.className = 'ph ph-check copy-icon text-success';
        setTimeout(() => {
          if (copyIcon) copyIcon.className = 'ph ph-copy copy-icon';
        }, 1500);
      }
      if (typeof showNotification === 'function') {
        showNotification(cnText('IP copied: ', 'Đã sao chép IP: ') + ip, 'success');
      }
    };
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(ip).then(doFeedback).catch(() => {
        fallbackCopy(ip, doFeedback);
      });
    } else {
      fallbackCopy(ip, doFeedback);
    }
  }

  function fallbackCopy(text, cb) {
    const ta = document.createElement('textarea');
    ta.value = text;
    ta.style.position = 'fixed';
    ta.style.opacity = '0';
    document.body.appendChild(ta);
    ta.select();
    try {
      document.execCommand('copy');
      if (cb) cb();
    } catch (e) {}
    document.body.removeChild(ta);
  }

  function openWhatsNewModal() {
    let modal = document.getElementById('whatsNewModal');
    if (!modal) {
      modal = document.createElement('div');
      modal.id = 'whatsNewModal';
      modal.className = 'modal-backdrop show';
      modal.innerHTML = `
        <div class="modal" style="max-width: 580px; width: 92%;">
          <div class="modal-header" style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 1rem; border-bottom: 1px solid hsl(var(--border)); padding-bottom: 0.75rem;">
            <h3 style="margin: 0; display: flex; align-items: center; gap: 0.5rem; font-size: 1.15rem;">
              <i class="ph ph-sparkle text-primary" style="color: #10b981;"></i> ${cnText("What's new in v2.0.0", 'Có gì mới trong v2.0.0')}
            </h3>
            <button class="btn-icon" onclick="closeWhatsNewModal()" aria-label="${cnText('Close', 'Đóng')}" style="background: none; border: none; cursor: pointer; font-size: 1.25rem; color: hsl(var(--muted-foreground));">
              <i class="ph ph-x"></i>
            </button>
          </div>
          <div class="modal-body" style="font-size: 0.9rem; line-height: 1.6; max-height: 65vh; overflow-y: auto; padding-right: 0.5rem;">
            <div style="margin-bottom: 1rem; padding: 0.75rem; background: hsl(var(--secondary) / 0.5); border-radius: 8px; border-left: 3px solid #10b981;">
              <strong>${cnText('v2.0.0 (beta) for Seeed reTerminal Sticky:', 'Bản v2.0.0 (beta) cho Seeed reTerminal Sticky:')}</strong><br>
              ${cnText('The Sticky port moves onto CrossNext 2.0.3: new reader engine and library, Bluetooth remotes, Focus and more, with every Sticky feature kept.', 'Bản Sticky chuyển lên nền CrossNext 2.0.3: engine đọc và thư viện mới, remote Bluetooth, Focus và nhiều hơn nữa, vẫn giữ mọi tính năng riêng của Sticky.')}
            </div>
            <ul style="padding-left: 1.25rem; margin: 0; display: flex; flex-direction: column; gap: 0.75rem;">
              <li><strong>${cnText('📖 Reader & library:', '📖 Đọc sách & thư viện:')}</strong> ${cnText('New EPUB engine (Classic 1.4 still available), per-book text settings, favourites and collections.', 'Engine EPUB mới (vẫn chọn được Classic 1.4), cài đặt chữ riêng cho từng sách, yêu thích và bộ sưu tập.')}</li>
              <li><strong>${cnText('🎮 Bluetooth remotes:', '🎮 Remote Bluetooth:')}</strong> ${cnText('Pair a BLE page turner, map short presses and holds, and add Bluetooth to Quick settings.', 'Ghép remote lật trang BLE, gán bấm ngắn/giữ riêng, thêm Bluetooth vào Quick settings.')}</li>
              <li><strong>${cnText('⏱️ Focus & statistics:', '⏱️ Focus & thống kê:')}</strong> ${cnText('Focus sessions from the App Store and reading statistics by day, month and year.', 'Phiên tập trung Focus từ App Store và thống kê đọc theo ngày, tháng, năm.')}</li>
              <li><strong>${cnText('🧩 Plugins:', '🧩 Plugin:')}</strong> ${cnText('Plugin details, compatibility checks and hold-to-uninstall; cloud folders keep working.', 'Trang chi tiết plugin, kiểm tra tương thích, giữ để gỡ; duyệt thư mục cloud vẫn hoạt động.')}</li>
              <li><strong>${cnText('⚠️ Beta:', '⚠️ Beta:')}</strong> ${cnText('Install the CrossNext 2.0 Resources before updating. PET and Reading Quest saves start over.', 'Cài gói Resources của CrossNext 2.0 trước khi cập nhật. Dữ liệu PET và Reading Quest sẽ bắt đầu lại.')}</li>
            </ul>
          </div>
          <div class="modal-actions" style="margin-top: 1.25rem; display: flex; justify-content: flex-end;">
            <button class="btn btn-primary" onclick="closeWhatsNewModal()" style="min-width: 100px;">${cnText('Got it', 'Đã hiểu')}</button>
          </div>
        </div>
      `;
      modal.onclick = function(e) {
        if (e.target === modal) closeWhatsNewModal();
      };
      document.body.appendChild(modal);
    } else {
      modal.style.display = 'flex';
      modal.classList.add('show');
    }
  }

  function closeWhatsNewModal() {
    const modal = document.getElementById('whatsNewModal');
    if (modal) {
      modal.style.display = 'none';
      modal.classList.remove('show');
    }
  }


