// ============================================================
// AuctionHub — общий код для всех страниц
// ============================================================

const api = {
    async request(method, path, body = null) {
        const headers = {
            'Content-Type': 'application/json'
        };

        const token = sessionStorage.getItem('auctionhub_token');

        if (token) {
            headers['Authorization'] = `Bearer ${token}`;
        }

        const res = await fetch(path, {
            method,
            headers,
            body: body ? JSON.stringify(body) : null,
        });

        const data = await res.json().catch(() => ({}));

        return {
            ok: res.ok,
            status: res.status,
            data
        };
    },
};

let currentUser = null;

function getStatusLabel(status) {
    const labels = {
        draft: 'Черновик',
        active: 'Опубликован',
        cancelled: 'Отменён',
        finished: 'Завершён'
    };
    return labels[status] || status;
}

function showMessage(el, text, isError) {
    if (!el) return;
    el.textContent = text;
    el.className = 'message ' + (isError ? 'error' : 'success');

    setTimeout(() => {
        el.className = 'message';
    }, 5000);
}

function updateAccountUI() {
    const accountButton = document.getElementById('account-button');
    const accountAvatar = document.getElementById('account-avatar');
    const accountName = document.getElementById('account-name');
    const menuAvatar = document.getElementById('account-menu-avatar');
    const menuEmail = document.getElementById('account-menu-email');
    const menuRole = document.getElementById('account-menu-role');
    const sellerNavLink = document.getElementById('seller-nav-link');

    if (!accountButton) return;

    if (!currentUser) {
        accountButton.hidden = true;
        if (sellerNavLink) sellerNavLink.hidden = true;
        return;
    }

    const initial = (currentUser.email || 'A').charAt(0).toUpperCase();

    accountButton.hidden = false;
    if (accountAvatar) accountAvatar.textContent = initial;
    if (menuAvatar) menuAvatar.textContent = initial;
    if (accountName) accountName.textContent = currentUser.email || 'Аккаунт';
    if (menuEmail) menuEmail.textContent = currentUser.email || 'Пользователь';

    const roleLabels = {
        seller: 'Продавец',
        bidder: 'Участник',
        moderator: 'Модератор'
    };

    if (menuRole) menuRole.textContent = roleLabels[currentUser.role] || currentUser.role;

    if (sellerNavLink) {
        sellerNavLink.hidden = currentUser.role !== 'seller';
    }
}

async function restoreSession() {
    const token = sessionStorage.getItem('auctionhub_token');

    if (!token) {
        updateAccountUI();
        return;
    }

    const { ok, data } = await api.request('GET', '/api/auth/me');

    if (!ok) {
        sessionStorage.removeItem('auctionhub_token');
        updateAccountUI();
        return;
    }

    currentUser = data;
    updateAccountUI();
}

async function logout() {
    const token = sessionStorage.getItem('auctionhub_token');

    if (token) {
        await api.request('POST', '/api/auth/logout');
    }

    sessionStorage.removeItem('auctionhub_token');
    currentUser = null;
    updateAccountUI();

    window.location.href = '/';
}

// --- Обработчики меню аккаунта ---

document.addEventListener('DOMContentLoaded', () => {
    const accountButton = document.getElementById('account-button');
    const accountMenu = document.getElementById('account-menu');
    const accountLogout = document.getElementById('account-logout');

    if (accountButton && accountMenu) {
        accountButton.addEventListener('click', () => {
            accountMenu.hidden = !accountMenu.hidden;
        });

        document.addEventListener('click', (e) => {
            if (!e.target.closest('.header-account')) {
                accountMenu.hidden = true;
            }
        });
    }

    if (accountLogout) {
        accountLogout.addEventListener('click', logout);
    }

    restoreSession();
});
