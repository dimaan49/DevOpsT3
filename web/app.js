// ============================================================
// AuctionHub — страница входа
// ============================================================

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
        showMessage(msgEl, data.message || 'Ошибка входа', true);
        return;
    }

    sessionStorage.setItem('auctionhub_token', data.token);
    currentUser = data.user;

    showMessage(msgEl, `Выполнен вход: ${data.user.email}`, false);

    // Редирект на страницу аукционов
    setTimeout(() => {
        window.location.href = '/auctions.html';
    }, 500);
});

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
        showMessage(msgEl, data.message || 'Ошибка регистрации', true);
        return;
    }

    showMessage(msgEl, `Зарегистрирован: ${data.email}`, false);
});

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
