const api = {
    async request(method, path, body = null, userId = null) {
        const headers = { 'Content-Type': 'application/json' };
        if (userId) headers['X-User-Id'] = userId;

        const res = await fetch(path, {
            method,
            headers,
            body: body ? JSON.stringify(body) : null,
        });

        const data = await res.json().catch(() => ({}));
        return { ok: res.ok, status: res.status, data };
    },
};

let currentUser = null;

function showMessage(el, text, isError) {
    el.textContent = text;
    el.className = 'message ' + (isError ? 'error' : 'success');
    setTimeout(() => { el.className = 'message'; }, 5000);
}

// --- Регистрация ---

document.getElementById('user-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    const email = document.getElementById('email').value;
    const password = document.getElementById('password').value;
    const role = document.getElementById('role').value;
    const ageConfirmed = document.getElementById('age-confirmed').checked;

    const msgEl = document.getElementById('user-message');
    const { ok, data } = await api.request('POST', '/api/users', {
        email, password, role, age_confirmed: ageConfirmed,
    });

    if (!ok) {
        showMessage(msgEl, data.message || 'Ошибка регистрации', true);
        return;
    }

    currentUser = data;
    showMessage(msgEl, `Зарегистрирован: ${data.email} (id=${data.id})`, false);
    document.getElementById('user-info').textContent =
        `${data.email} — роль: ${data.role} (id=${data.id})`;
    loadAuctions();
});

// --- Аукционы ---

async function loadAuctions() {
    const { ok, data } = await api.request('GET', '/api/auctions');
    const list = document.getElementById('auctions-list');
    list.innerHTML = '';

    if (!ok) {
        list.textContent = 'Не удалось загрузить аукционы';
        return;
    }

    if (!data.items || data.items.length === 0) {
        list.textContent = 'Аукционов пока нет.';
        return;
    }

    for (const a of data.items) {
        const card = document.createElement('div');
        card.className = 'auction-card';
        card.innerHTML = `
            <strong>${a.title}</strong>
            <span class="status ${a.status}">${a.status}</span>
            <div>${a.description || ''}</div>
            <div>Шаг: ${a.step} | Старт: ${a.start_price}</div>
        `;
        card.addEventListener('click', () => loadAuctionDetail(a.id));
        list.appendChild(card);
    }
}

document.getElementById('refresh-auctions')
    .addEventListener('click', loadAuctions);

// --- Детали аукциона и лоты ---

async function loadAuctionDetail(auctionId) {
    const { ok, data } = await api.request('GET', `/api/auctions/${auctionId}`);
    if (!ok) return;

    if (!data.lots || data.lots.length === 0) {
        alert('В этом аукционе пока нет лотов');
        return;
    }

    // Берём первый лот для демонстрации.
    const lot = data.lots[0];
    openLot(lot);
}

function openLot(lot) {
    document.getElementById('lot-section').hidden = false;
    document.getElementById('lot-title').textContent = lot.title;
    document.getElementById('lot-details').textContent =
        `${lot.description || ''} — старт: ${lot.start_price}`;
    document.getElementById('bid-amount').dataset.lotId = lot.id;
    loadBids(lot.id);
}

// --- Ставки ---

document.getElementById('bid-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    if (!currentUser) {
        showMessage(document.getElementById('bid-message'),
            'Сначала зарегистрируйтесь', true);
        return;
    }

    const lotId = document.getElementById('bid-amount').dataset.lotId;
    const amount = parseFloat(document.getElementById('bid-amount').value);

    const msgEl = document.getElementById('bid-message');
    const { ok, data } = await api.request(
        'POST', `/api/lots/${lotId}/bids`,
        { amount }, currentUser.id
    );

    if (!ok) {
        showMessage(msgEl, data.message || 'Ошибка', true);
        return;
    }

    showMessage(msgEl, `Ставка принята: ${data.amount}`, false);
    loadBids(lotId);
});

async function loadBids(lotId) {
    const { ok, data } = await api.request('GET', `/api/lots/${lotId}/bids`);
    const list = document.getElementById('bids-list');
    list.innerHTML = '';

    if (!ok) return;

    for (const b of data.items) {
        const row = document.createElement('div');
        row.className = 'bid-row';
        row.textContent =
            `${b.amount} — bidder #${b.bidder_id} (${b.created_at})`;
        list.appendChild(row);
    }

    if (data.items.length === 0) {
        list.textContent = 'Ставок пока нет.';
    }
}

// --- Стартовая загрузка ---
loadAuctions();
