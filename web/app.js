
function setAuthView(isAuthenticated) {
    const userSection = document.getElementById('user-section');
    const accountButton = document.getElementById('account-button');
    const accountMenu = document.getElementById('account-menu');
    const accountName = document.getElementById('account-name');
    const accountEmail = document.getElementById('account-menu-email');
    const accountRole = document.getElementById('account-menu-role');
    const accountAvatar = document.getElementById('account-avatar');
    const accountMenuAvatar = document.getElementById('account-menu-avatar');

    if (userSection) {
        userSection.classList.toggle('authenticated', isAuthenticated);
    }

    if (!isAuthenticated) {
        if (accountButton) accountButton.hidden = true;
        if (accountMenu) accountMenu.hidden = true;
        if (accountName) accountName.textContent = '';
        if (accountEmail) accountEmail.textContent = '';
        if (accountRole) accountRole.textContent = '';
        if (accountAvatar) accountAvatar.textContent = '';
        if (accountMenuAvatar) accountMenuAvatar.textContent = '';
    }
}

const api = {
    async request(method, path, body = null, userId = null) {
        const headers = {
            'Content-Type': 'application/json'
        };

        if (userId) {
            headers['X-User-Id'] = userId;
        }

        const token = localStorage.getItem('auctionhub_token');

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
let selectedAuctionId = null;

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
    el.textContent = text;
    el.className = 'message ' + (isError ? 'error' : 'success');

    setTimeout(() => {
        el.className = 'message';
    }, 5000);
}


// --- Вход ---
document.getElementById('login-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    const email = document.getElementById('login-email').value.trim();
    const password = document.getElementById('login-password').value;
    const msgEl = document.getElementById('login-message');

    const { ok, data } = await api.request('POST', '/api/auth/login', {
        email,
        password
    });

    if (!ok) {
        showMessage(
            msgEl,
            data.message || 'Ошибка входа',
            true
        );
        return;
    }

    localStorage.setItem('auctionhub_token', data.token);
    currentUser = data.user;
        setAuthView(true);

    showMessage(
        msgEl,
        `Выполнен вход: ${data.user.email}`,
        false
    );

    document.getElementById('user-info').textContent =
        `${data.user.email} — роль: ${data.user.role} (id=${data.user.id})`;

    document.getElementById('login-block').hidden = true;
    document.getElementById('register-block').hidden = true;
    document.getElementById('logout-button').hidden = false;

    loadAuctions();
    updateSellerInterface();
    updateAccountUI();
});


// --- Восстановление сессии ---
async function restoreSession() {
    const token = localStorage.getItem('auctionhub_token');

    if (!token) {
        return;
    }

    const { ok, data } = await api.request('GET', '/api/auth/me');

    if (!ok) {
        localStorage.removeItem('auctionhub_token');
        return;
    }

    currentUser = data;

    document.getElementById('user-info').textContent =
        `${data.email} — роль: ${data.role} (id=${data.id})`;

    document.getElementById('login-block').hidden = true;
    document.getElementById('register-block').hidden = true;
    document.getElementById('logout-button').hidden = false;

    loadAuctions();
    updateSellerInterface();
    updateAccountUI();
}


// --- Выход ---
document.getElementById('logout-button').addEventListener('click', async () => {
    const token = localStorage.getItem('auctionhub_token');

    if (token) {
        await api.request('POST', '/api/auth/logout');
    }

    localStorage.removeItem('auctionhub_token');
    currentUser = null;
    selectedAuctionId = null;

    document.getElementById('user-info').textContent = 'Не авторизован';
    document.getElementById('login-block').hidden = false;
    document.getElementById('register-block').hidden = false;
    document.getElementById('logout-button').hidden = true;

    document.getElementById('seller-section').hidden = true;
    document.getElementById('auction-management').hidden = true;
});

// --- UI аккаунта ---
function updateAccountUI() {
    const accountButton = document.getElementById('account-button');
    const accountAvatar = document.getElementById('account-avatar');
    const accountName = document.getElementById('account-name');
    const menuAvatar = document.getElementById('account-menu-avatar');
    const menuEmail = document.getElementById('account-menu-email');
    const menuRole = document.getElementById('account-menu-role');
    const loginBlock = document.getElementById('login-block');
    const registerBlock = document.getElementById('register-block');

    if (!accountButton) {
        return;
    }

    if (!currentUser) {
        accountButton.hidden = true;
        loginBlock.hidden = false;
        registerBlock.hidden = false;
        return;
    }

    const initial = (currentUser.email || 'A').charAt(0).toUpperCase();

    accountButton.hidden = false;
    accountAvatar.textContent = initial;
    menuAvatar.textContent = initial;
    accountName.textContent = currentUser.email || 'Аккаунт';
    menuEmail.textContent = currentUser.email || 'Пользователь';
    menuRole.textContent =
        currentUser.role === 'seller' ? 'Продавец' : 'Участник';

    loginBlock.hidden = true;
    registerBlock.hidden = true;
}

const accountButton = document.getElementById('account-button');
const accountMenu = document.getElementById('account-menu');

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

const showRegister = document.getElementById('show-register');
const showLogin = document.getElementById('show-login');

if (showRegister) {
    showRegister.addEventListener('click', (e) => {
        e.preventDefault();
        document.getElementById('login-block').hidden = true;
        document.getElementById('register-block').hidden = false;
    });
}

if (showLogin) {
    showLogin.addEventListener('click', (e) => {
        e.preventDefault();
        document.getElementById('register-block').hidden = true;
        document.getElementById('login-block').hidden = false;
    });
}

const accountLogout = document.getElementById('account-logout');

if (accountLogout) {
    accountLogout.addEventListener('click', async () => {
        const token = localStorage.getItem('auctionhub_token');

        if (token) {
            await api.request('POST', '/api/auth/logout');
        }

        localStorage.removeItem('auctionhub_token');
        currentUser = null;
        selectedAuctionId = null;

        document.getElementById('user-info').textContent = 'Не авторизован';
        document.getElementById('account-menu').hidden = true;
        document.getElementById('seller-section').hidden = true;
        document.getElementById('auction-management').hidden = true;

        updateAccountUI();
    });
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
        email,
        password,
        role,
        age_confirmed: ageConfirmed,
    });

    if (!ok) {
        showMessage(
            msgEl,
            data.message || 'Ошибка регистрации',
            true
        );
        return;
    }

    currentUser = data;

    showMessage(
        msgEl,
        `Зарегистрирован: ${data.email} (id=${data.id})`,
        false
    );

    document.getElementById('user-info').textContent =
        `${data.email} — роль: ${data.role} (id=${data.id})`;

    loadAuctions();
    updateSellerInterface();
});


// --- Управление продавца ---

function updateSellerInterface() {
    const sellerSection = document.getElementById('seller-section');

    if (!currentUser || currentUser.role !== 'seller') {
        sellerSection.hidden = true;
        return;
    }

    sellerSection.hidden = false;
    loadMyAuctions();
}

async function loadMyAuctions() {
    if (!currentUser || currentUser.role !== 'seller') {
        return;
    }

    const list = document.getElementById('my-auctions-list');
    list.innerHTML = 'Загрузка...';

    const { ok, data } = await api.request(
        'GET',
        '/api/auctions'
    );

    if (!ok) {
        list.textContent =
            data.message || 'Не удалось загрузить аукционы';
        return;
    }

    list.innerHTML = '';

    const myAuctions = (data.items || []).filter(
        auction => auction.seller_id === currentUser.id
    );

    if (myAuctions.length === 0) {
        list.textContent = 'У вас пока нет аукционов.';
        return;
    }

    for (const auction of myAuctions) {
        const card = document.createElement('div');
        card.className = 'auction-card';

        card.innerHTML = `
            <strong>${auction.title}</strong>
            <span class="status ${auction.status}">
                ${getStatusLabel(auction.status)}
            </span>
            <div>${auction.description || ''}</div>
            <div>
                Шаг: ${auction.step}
                | Старт: ${auction.start_price}
            </div>
            <button type="button">
                Управлять
            </button>
        `;

        card.querySelector('button').addEventListener(
            'click',
            (event) => {
                event.stopPropagation();
                selectAuctionForManagement(auction.id);
            }
        );

        list.appendChild(card);
    }
}

async function selectAuctionForManagement(auctionId) {
    const { ok, data } = await api.request(
        'GET',
        `/api/auctions/${auctionId}`
    );

    if (!ok) {
        showMessage(
            document.getElementById('auction-message'),
            data.message || 'Не удалось загрузить аукцион',
            true
        );
        return;
    }

    selectedAuctionId = auctionId;

    const management =
        document.getElementById('auction-management');

    const details =
        document.getElementById('selected-auction-details');

    management.hidden = false;

    details.innerHTML = `
        <strong>${data.title}</strong>
        <div>Статус: ${getStatusLabel(data.status)}</div>
        <div>Описание: ${data.description || ''}</div>
        <div>Шаг ставки: ${data.step}</div>
        <div>Начальная цена: ${data.start_price}</div>
        <div>Лотов: ${(data.lots || []).length}</div>
    `;

    document.getElementById('publish-auction').disabled =
        data.status !== 'draft';

    document.getElementById('cancel-auction').disabled =
        data.status === 'finished' ||
        data.status === 'cancelled';
}


// --- Создание аукциона ---

document.getElementById('auction-form').addEventListener(
    'submit',
    async (e) => {
        e.preventDefault();

        if (!currentUser || currentUser.role !== 'seller') {
            showMessage(
                document.getElementById('auction-message'),
                'Только продавец может создать аукцион',
                true
            );
            return;
        }

        const title =
            document.getElementById('auction-title').value.trim();

        const description =
            document.getElementById('auction-description').value.trim();

        const step =
            parseFloat(
                document.getElementById('auction-step').value
            );

        const startPrice =
            parseFloat(
                document.getElementById('auction-start-price').value
            );

        const { ok, data } = await api.request(
            'POST',
            '/api/auctions',
            {
                title,
                description,
                step,
                start_price: startPrice
            },
            currentUser.id
        );

        const msgEl =
            document.getElementById('auction-message');

        if (!ok) {
            showMessage(
                msgEl,
                data.message || 'Не удалось создать аукцион',
                true
            );
            return;
        }

        showMessage(
            msgEl,
            `Аукцион создан: ${data.title || title}`,
            false
        );

        document.getElementById('auction-form').reset();

        await loadMyAuctions();
        await loadAuctions();
    }
);


// --- Добавление лота ---

document.getElementById('lot-form').addEventListener(
    'submit',
    async (e) => {
        e.preventDefault();

        if (!selectedAuctionId) {
            showMessage(
                document.getElementById('lot-management-message'),
                'Сначала выберите аукцион',
                true
            );
            return;
        }

        const title =
            document.getElementById('new-lot-title').value.trim();

        const description =
            document.getElementById('new-lot-description').value.trim();

        const startPrice =
            parseFloat(
                document.getElementById('new-lot-start-price').value
            );

        const { ok, data } = await api.request(
            'POST',
            `/api/auctions/${selectedAuctionId}/lots`,
            {
                title,
                description,
                start_price: startPrice
            },
            currentUser.id
        );

        const msgEl =
            document.getElementById('lot-management-message');

        if (!ok) {
            showMessage(
                msgEl,
                data.message || 'Не удалось добавить лот',
                true
            );
            return;
        }

        showMessage(
            msgEl,
            `Лот добавлен: ${data.title || title}`,
            false
        );

        document.getElementById('lot-form').reset();

        await selectAuctionForManagement(selectedAuctionId);
        await loadAuctions();
    }
);


// --- Публикация аукциона ---

document.getElementById('publish-auction').addEventListener(
    'click',
    async () => {
        if (!selectedAuctionId || !currentUser) {
            return;
        }

        const { ok, data } = await api.request(
            'POST',
            `/api/auctions/${selectedAuctionId}/publish`,
            null,
            currentUser.id
        );

        const msgEl =
            document.getElementById('auction-message');

        if (!ok) {
            showMessage(
                msgEl,
                data.message || 'Не удалось опубликовать аукцион',
                true
            );
            return;
        }

        showMessage(
            msgEl,
            'Аукцион успешно опубликован',
            false
        );

        await selectAuctionForManagement(selectedAuctionId);
        await loadMyAuctions();
        await loadAuctions();
    }
);


// --- Отмена аукциона ---

document.getElementById('cancel-auction').addEventListener(
    'click',
    async () => {
        if (!selectedAuctionId || !currentUser) {
            return;
        }

        if (!confirm('Вы действительно хотите отменить аукцион?')) {
            return;
        }

        const { ok, data } = await api.request(
            'POST',
            `/api/auctions/${selectedAuctionId}/cancel`,
            null,
            currentUser.id
        );

        const msgEl =
            document.getElementById('auction-message');

        if (!ok) {
            showMessage(
                msgEl,
                data.message || 'Не удалось отменить аукцион',
                true
            );
            return;
        }

        showMessage(
            msgEl,
            'Аукцион отменён',
            false
        );

        await selectAuctionForManagement(selectedAuctionId);
        await loadMyAuctions();
        await loadAuctions();
    }
);



// --- Удаление аукциона ---

document.getElementById('delete-auction').addEventListener(
    'click',
    async () => {
        if (!selectedAuctionId || !currentUser) {
            return;
        }

        if (!confirm('Вы действительно хотите удалить аукцион?')) {
            return;
        }

        const { ok, data } = await api.request(
            'DELETE',
            `/api/auctions/${selectedAuctionId}`,
            null,
            currentUser.id
        );

        const msgEl =
            document.getElementById('auction-message');

        if (!ok) {
            showMessage(
                msgEl,
                data.message || 'Не удалось удалить аукцион',
                true
            );
            return;
        }

        showMessage(
            msgEl,
            'Аукцион удалён',
            false
        );

        selectedAuctionId = null;

        document.getElementById('selected-auction-details').hidden = true;

        await loadMyAuctions();
        await loadAuctions();
    }
);

// --- Аукционы ---

async function loadAuctions() {
    const { ok, data } = await api.request(
        'GET',
        '/api/auctions'
    );

    const list =
        document.getElementById('auctions-list');

    list.innerHTML = '';

    if (!ok) {
        list.textContent =
            'Не удалось загрузить аукционы';
        return;
    }

    if (!data.items || data.items.length === 0) {
        list.textContent =
            'Аукционов пока нет.';
        return;
    }

    for (const a of data.items) {
        const card =
            document.createElement('div');

        card.className = 'auction-card';

        card.innerHTML = `
            <strong>${a.title}</strong>
            <span class="status ${a.status}">
                ${getStatusLabel(a.status)}
            </span>
            <div>${a.description || ''}</div>
            <div>
                Шаг: ${a.step}
                | Старт: ${a.start_price}
            </div>
        `;

        card.addEventListener(
            'click',
            () => loadAuctionDetail(a.id)
        );

        list.appendChild(card);
    }
}

document.getElementById('refresh-auctions')
    .addEventListener(
        'click',
        loadAuctions
    );


// --- Детали аукциона и лоты ---

async function loadAuctionDetail(auctionId) {
    const { ok, data } = await api.request(
        'GET',
        `/api/auctions/${auctionId}`
    );

    if (!ok) {
        return;
    }

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

        card.querySelector('button').addEventListener(
            'click',
            () => openLot(lot)
        );

        lotDetails.appendChild(card);
    }

    openLot(data.lots[0]);
}

function openLot(lot) {
    document.getElementById('bid-amount').dataset.lotId =
        lot.id;

    document.getElementById('bid-amount').value = '';

    document.getElementById('bid-message').textContent = '';

    loadBids(lot.id);
}


// --- Ставки ---

document.getElementById('bid-form').addEventListener(
    'submit',
    async (e) => {
        e.preventDefault();

        if (!currentUser) {
            showMessage(
                document.getElementById('bid-message'),
                'Сначала зарегистрируйтесь',
                true
            );
            return;
        }

        const lotId =
            document.getElementById('bid-amount')
                .dataset.lotId;

        const amount =
            parseFloat(
                document.getElementById('bid-amount').value
            );

        const msgEl =
            document.getElementById('bid-message');

        const { ok, data } = await api.request(
            'POST',
            `/api/lots/${lotId}/bids`,
            { amount },
            currentUser.id
        );

        if (!ok) {
            showMessage(
                msgEl,
                data.message || 'Ошибка',
                true
            );
            return;
        }

        showMessage(
            msgEl,
            `Ставка принята: ${data.amount}`,
            false
        );

        loadBids(lotId);
    }
);

async function loadBids(lotId) {
    const { ok, data } = await api.request(
        'GET',
        `/api/lots/${lotId}/bids`
    );

    const list =
        document.getElementById('bids-list');

    list.innerHTML = '';

    if (!ok) {
        return;
    }

    for (const b of data.items) {
        const row =
            document.createElement('div');

        row.className = 'bid-row';

        row.textContent =
            `${b.amount} — участник #${b.bidder_id} (${b.created_at})`;

        list.appendChild(row);
    }

    if (data.items.length === 0) {
        list.textContent =
            'Ставок пока нет.';
    }
}


// --- Стартовая загрузка ---

restoreSession();
