// ============================================================
// AuctionHub — страница аукционов
// ============================================================

async function loadAuctions(query = '') {
    const path = query
        ? `/api/auctions?q=${encodeURIComponent(query)}`
        : '/api/auctions';

    const { ok, data } = await api.request('GET', path);
    const list = document.getElementById('auctions-list');
    list.innerHTML = '';

    if (!ok) {
        list.textContent = 'Не удалось загрузить аукционы';
        return;
    }

    if (!data.items || data.items.length === 0) {
        list.textContent = query ? 'Ничего не найдено.' : 'Аукционов пока нет.';
        return;
    }

    for (const a of data.items) {
        const card = document.createElement('div');
        card.className = 'auction-card';

        card.innerHTML = `
            <strong>${a.title}</strong>
            <span class="status ${a.status}">${getStatusLabel(a.status)}</span>
            <div>${a.description || ''}</div>
            <div>Шаг: ${a.step} | Старт: ${a.start_price}</div>
        `;

        card.addEventListener('click', () => loadAuctionDetail(a.id));
        list.appendChild(card);
    }
}

document.getElementById('auction-search-button').addEventListener('click', () => {
    const q = document.getElementById('auction-search').value.trim();
    loadAuctions(q);
});

document.getElementById('auction-search-reset').addEventListener('click', () => {
    document.getElementById('auction-search').value = '';
    loadAuctions();
});

document.getElementById('auction-search').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        const q = document.getElementById('auction-search').value.trim();
        loadAuctions(q);
    }
});

async function loadAuctionDetail(auctionId) {
    const { ok, data } = await api.request('GET', `/api/auctions/${auctionId}`);
    if (!ok) return;

    const lotSection = document.getElementById('lot-section');
    const lotTitle = document.getElementById('lot-title');
    const lotDetails = document.getElementById('lot-details');

    lotSection.hidden = false;
    lotTitle.textContent = data.title;
    lotDetails.innerHTML = '';

    if (!data.lots || data.lots.length === 0) {
        lotDetails.textContent = 'В этом аукционе пока нет лотов.';
        document.getElementById('bid-form').hidden = true;
        document.getElementById('bids-list').innerHTML = '';
        return;
    }

    document.getElementById('bid-form').hidden = false;

    for (const lot of data.lots) {
        const card = document.createElement('div');
        card.className = 'lot-card';
        card.innerHTML = `
            <strong>${lot.title}</strong>
            <div>${lot.description || ''}</div>
            <div>Начальная цена: ${lot.start_price}</div>
            <button type="button">Смотреть ставки</button>
        `;
        card.querySelector('button').addEventListener('click', () => openLot(lot));
        lotDetails.appendChild(card);
    }

    openLot(data.lots[0]);
}

function openLot(lot) {
    document.getElementById('bid-amount').dataset.lotId = lot.id;
    document.getElementById('bid-amount').value = '';
    document.getElementById('bid-message').textContent = '';
    loadBids(lot.id);
}

document.getElementById('bid-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    if (!currentUser) {
        showMessage(document.getElementById('bid-message'), 'Сначала войдите', true);
        return;
    }

    const lotId = document.getElementById('bid-amount').dataset.lotId;
    const amount = parseFloat(document.getElementById('bid-amount').value);
    const msgEl = document.getElementById('bid-message');

    const { ok, data } = await api.request('POST', `/api/lots/${lotId}/bids`, { amount });

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
        row.textContent = `${b.amount} — участник #${b.bidder_id} (${b.created_at})`;
        list.appendChild(row);
    }

    if (data.items.length === 0) {
        list.textContent = 'Ставок пока нет.';
    }
}

loadAuctions();
