// ============================================================
// AuctionHub — страница «Мои аукционы» (только для продавца)
// ============================================================

let selectedAuctionId = null;

async function loadMyAuctions() {
    if (!currentUser || currentUser.role !== 'seller') return;

    const list = document.getElementById('my-auctions-list');
    list.innerHTML = 'Загрузка...';

    const { ok, data } = await api.request('GET', '/api/auctions');

    if (!ok) {
        list.textContent = data.message || 'Не удалось загрузить аукционы';
        return;
    }

    list.innerHTML = '';
    const myAuctions = (data.items || []).filter(a => a.seller_id === currentUser.id);

    if (myAuctions.length === 0) {
        list.textContent = 'У вас пока нет аукционов.';
        return;
    }

    for (const auction of myAuctions) {
        const card = document.createElement('div');
        card.className = 'auction-card';

        card.innerHTML = `
            <strong>${auction.title}</strong>
            <span class="status ${auction.status}">${getStatusLabel(auction.status)}</span>
            <div>${auction.description || ''}</div>
            <div>Шаг: ${auction.step} | Старт: ${auction.start_price}</div>
            <button type="button">Управлять</button>
        `;

        card.querySelector('button').addEventListener('click', (event) => {
            event.stopPropagation();
            selectAuctionForManagement(auction.id);
        });

        list.appendChild(card);
    }
}

async function selectAuctionForManagement(auctionId) {
    const { ok, data } = await api.request('GET', `/api/auctions/${auctionId}`);

    if (!ok) {
        showMessage(document.getElementById('auction-message'),
            data.message || 'Не удалось загрузить аукцион', true);
        return;
    }

    selectedAuctionId = auctionId;

    const management = document.getElementById('auction-management');
    const details = document.getElementById('selected-auction-details');
    management.hidden = false;

    details.innerHTML = `
        <strong>${data.title}</strong>
        <div>Статус: ${getStatusLabel(data.status)}</div>
        <div>Описание: ${data.description || ''}</div>
        <div>Шаг ставки: ${data.step}</div>
        <div>Начальная цена: ${data.start_price}</div>
        <div>Лотов: ${(data.lots || []).length}</div>
    `;

    document.getElementById('publish-auction').disabled = data.status !== 'draft';
    document.getElementById('cancel-auction').disabled =
        data.status === 'finished' || data.status === 'cancelled';
}

document.getElementById('auction-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    if (!currentUser || currentUser.role !== 'seller') {
        showMessage(document.getElementById('auction-message'),
            'Только продавец может создать аукцион', true);
        return;
    }

    const title = document.getElementById('auction-title').value.trim();
    const description = document.getElementById('auction-description').value.trim();
    const step = parseFloat(document.getElementById('auction-step').value);
    const startPrice = parseFloat(document.getElementById('auction-start-price').value);

    const { ok, data } = await api.request('POST', '/api/auctions', {
        title, description, step, start_price: startPrice
    });

    const msgEl = document.getElementById('auction-message');

    if (!ok) {
        showMessage(msgEl, data.message || 'Не удалось создать аукцион', true);
        return;
    }

    showMessage(msgEl, `Аукцион создан: ${data.title || title}`, false);
    document.getElementById('auction-form').reset();
    await loadMyAuctions();
});

document.getElementById('lot-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    if (!selectedAuctionId) {
        showMessage(document.getElementById('lot-management-message'),
            'Сначала выберите аукцион', true);
        return;
    }

    const title = document.getElementById('new-lot-title').value.trim();
    const description = document.getElementById('new-lot-description').value.trim();
    const startPrice = parseFloat(document.getElementById('new-lot-start-price').value);

    const { ok, data } = await api.request('POST',
        `/api/auctions/${selectedAuctionId}/lots`,
        { title, description, start_price: startPrice });

    const msgEl = document.getElementById('lot-management-message');

    if (!ok) {
        showMessage(msgEl, data.message || 'Не удалось добавить лот', true);
        return;
    }

    showMessage(msgEl, `Лот добавлен: ${data.title || title}`, false);
    document.getElementById('lot-form').reset();
    await selectAuctionForManagement(selectedAuctionId);
});

document.getElementById('publish-auction').addEventListener('click', async () => {
    if (!selectedAuctionId || !currentUser) return;

    const { ok, data } = await api.request('POST',
        `/api/auctions/${selectedAuctionId}/publish`);

    const msgEl = document.getElementById('auction-message');

    if (!ok) {
        showMessage(msgEl, data.message || 'Не удалось опубликовать аукцион', true);
        return;
    }

    showMessage(msgEl, 'Аукцион успешно опубликован', false);
    await selectAuctionForManagement(selectedAuctionId);
    await loadMyAuctions();
});

document.getElementById('cancel-auction').addEventListener('click', async () => {
    if (!selectedAuctionId || !currentUser) return;
    if (!confirm('Вы действительно хотите отменить аукцион?')) return;

    const { ok, data } = await api.request('POST',
        `/api/auctions/${selectedAuctionId}/cancel`);

    const msgEl = document.getElementById('auction-message');

    if (!ok) {
        showMessage(msgEl, data.message || 'Не удалось отменить аукцион', true);
        return;
    }

    showMessage(msgEl, 'Аукцион отменён', false);
    await selectAuctionForManagement(selectedAuctionId);
    await loadMyAuctions();
});

document.getElementById('delete-auction').addEventListener('click', async () => {
    if (!selectedAuctionId || !currentUser) return;
    if (!confirm('Вы действительно хотите удалить аукцион?')) return;

    const { ok, data } = await api.request('DELETE',
        `/api/auctions/${selectedAuctionId}`);

    const msgEl = document.getElementById('auction-message');

    if (!ok) {
        showMessage(msgEl, data.message || 'Не удалось удалить аукцион', true);
        return;
    }

    showMessage(msgEl, 'Аукцион удалён', false);
    selectedAuctionId = null;
    document.getElementById('selected-auction-details').hidden = true;
    await loadMyAuctions();
});

// Загружаем аукционы только после restoreSession
document.addEventListener('DOMContentLoaded', () => {
    setTimeout(() => {
        if (currentUser && currentUser.role === 'seller') {
            loadMyAuctions();
        }
    }, 200);
});
