# Развёртывание AuctionHub

Описание ручных шагов, структуры `deploy/` и назначения каждого скрипта.

---

## Архитектура
Это лишь пример, можно настравивать любые ip адреса и сети.
Две виртуальные машины:

| Роль | IP | Назначение |
|------|-----|-----------|
| app | `192.168.56.101` | Приложение AuctionHub, HTTP API на 8080 |
| db  | `192.168.56.102` | PostgreSQL на 5432 |

Каждая VM имеет два сетевых адаптера:

- **NAT** — выход в интернет для `apt install`.
- **Host-only Adapter** — связь между VM и хостом. Сеть `192.168.56.0/24`, шлюз `192.168.56.1`.

---

## Ручные шаги до скриптов

Эти шаги выполняются вручную. Скрипты в `deploy/` их не автоматизируют.

### 1. Создание VM в VirtualBox

Создать две VM. Установить **Debian netinst** без графического окружения.

При установке:

- на шаге **Software selection** снять галочку с `Debian desktop environment` и всех DE;
- оставить только `SSH server` и `standard system utilities`;
- задать пароль `root` (для первичного входа по консоли).

### 2. Настройка сетевых адаптеров

В VirtualBox для каждой VM: **Settings → Network**.

- **Adapter 1:** `NAT`.
- **Adapter 2:** `Host-only Adapter`, выбрать сеть `vboxnet0`.

Если Host-only сети нет — создать через **File → Host Network Manager**:

- IPv4 Address: `192.168.56.1`
- IPv4 Mask: `255.255.255.0`

### 3. Загрузка VM и вход

Загрузить VM. Войти под `root` через консоль VirtualBox.

Проверить сетевые интерфейсы:

```bash
ip a 
```

Ожидается:

- `enp0s3` — NAT (`10.0.2.15/24`)
- `enp0s8` — Host-only (IP от DHCP, например `192.168.56.101`)

### 4. Настройка доступа и клонирование

На каждой VM:

1. Скопировать публичный ключ на VM под root:

```bash
ssh-copy-id -i ~/.ssh/id_ed25519.pub root@<IP>
```

Проверить вход без пароля:

```bash
ssh root@<IP>
```

2. Клонировать репозиторий:

```bash
apt update
apt install -y git
git clone -b feature/deploy-ext https://github.com/dimaan49/DevOpsT3.git /root/DevOpsT3
```

3. Запустить настройку общего окружения:

```bash
sudo bash /root/DevOpsT3/deploy/install.sh --common
```

Что делает `install.sh --common`:

- создаёт пользователя `admin`, добавляет в группу `sudo`;
- копирует SSH-ключ из `/root/.ssh/authorized_keys` в `/home/admin/.ssh/authorized_keys`;
- запрещает root-логин (`PermitRootLogin no`);
- отключает вход по паролю (`PasswordAuthentication no`);
- оставляет только вход по ключу.

4. **Проверить в новом терминале**, что вход под `admin` работает, прежде чем закрывать текущую сессию:

```bash
ssh admin@<IP>
```

Если пускает — можно работать под `admin`. Если нет — вернуться в текущую сессию root и разобраться, потом откатить `sshd_config` из `/etc/ssh/sshd_config.bak`.

Дальше всё делается под `admin` через `sudo`.


### 5. Клонирование репозитория

На каждой VM под `admin`:

```bash
git clone -b feature/deploy-ext https://github.com/dimaan49/DevOpsT3.git ~/DevOpsT3
```

Дальше все команды выполняются из `~/DevOpsT3`.

---

## Структура `deploy/`

```
deploy/
├── deploy.conf              — общий конфиг (IP, имена, URL репозитория)
├── install.sh               — обёртка с флагами --app / --db / --common
├── common/
│   ├── setup-ssh.sh         — hardening SSH: root-логин запрещён, ключи
│   └── setup-network.sh     — статический IP через NetworkManager
├── db/
│   ├── 01-install-postgres.sh     — установка PostgreSQL
│   ├── 02-create-db-user.sh       — SQL-пользователь dbuser и БД auctionhub
│   ├── 03-configure-postgres.sh   — postgresql.conf и pg_hba.conf
│   └── 04-firewall.sh             — UFW: 5432 только с app-IP
└── app/
    ├── auctionhub.service         — systemd-юнит
    ├── 01-install-deps.sh         — Qt6, CMake, Ninja, g++, libpq
    ├── 02-create-app-user.sh      — системный пользователь appuser
    ├── 03-setup-env.sh            — .env из шаблона, JWT-секрет через openssl
    ├── 04-clone-and-build.sh      — git clone + make build
    ├── 05-apply-schema.sh         — psql на db-сервер с schema.sql
    ├── 06-install-service.sh      — systemd-служба
    └── 07-firewall.sh             — UFW: 8080/tcp
```

### `deploy.conf`

Общие параметры для всех скриптов:

- `APP_IP`, `DB_IP` — статические IP.
- `ADMIN_USER`, `APP_USER` — имена пользователей.
- `APP_HOME`, `APP_DIR` — домашняя директория и директория кода.
- `DB_NAME`, `DB_USER`, `DB_PORT`, `DB_PASSWORD` — параметры PostgreSQL.
- `REPO_URL`, `REPO_BRANCH` — откуда клонировать.

Подключается через `source` в начале каждого скрипта.

### `install.sh`

Обёртка. Запускается с одним из флагов:

```bash
sudo bash deploy/install.sh --common
sudo bash deploy/install.sh --db
sudo bash deploy/install.sh --app
```

- `--common` — только `common/`.
- `--db` — `common/` + `db/`, передаёт `--role db` в `setup-network.sh`.
- `--app` — `common/` + `app/`, передаёт `--role app`.

Скрипты в подпапках выполняются в алфавитном порядке (отсюда префиксы `01-`, `02-`).

---

## Скрипты `common/`

### `setup-ssh.sh`

- Создаёт `admin` (если нет), добавляет в группу `sudo`.
- Копирует `/root/.ssh/authorized_keys` в `/home/admin/.ssh/authorized_keys`.
- Правит `/etc/ssh/sshd_config`: `PermitRootLogin no`, `PasswordAuthentication no`.
- Делает бэкап в `sshd_config.bak`.
- Перезапускает `ssh`.

### `setup-network.sh`

- Определяет NAT-интерфейс (через `ip route show default`).
- Определяет Host-only интерфейс (второй ethernet в NetworkManager).
- Берёт IP из `deploy.conf` по роли (`--role app` → `APP_IP`, `--role db` → `DB_IP`).
- Если IP пуст — спрашивает интерактивно.
- Настраивает статический IP через `nmcli connection modify`.

---

## Скрипты `db/`

### `01-install-postgres.sh`

- Устанавливает `postgresql`, `postgresql-contrib`.
- `systemctl enable postgresql`, `systemctl start postgresql`.

### `02-create-db-user.sh`

- Спрашивает пароль для `dbuser` (или берёт из `DB_PASSWORD`).
- Создаёт SQL-роль `dbuser` и БД `auctionhub` через `sudo -u postgres psql`.

### `03-configure-postgres.sh`

- Определяет директорию конфига (`/etc/postgresql/<ver>/main`).
- В `postgresql.conf` прописывает `listen_addresses = '<DB_IP>'`.
- В `pg_hba.conf` добавляет правило: `dbuser` с `APP_IP/32` → `auctionhub` через `scram-sha-256`.
- Делает бэкапы `.bak`.
- Перезапускает PostgreSQL.

### `04-firewall.sh`

- Устанавливает UFW.
- Разрешает SSH (22) и PostgreSQL (5432) **только с `APP_IP`**.

---

## Скрипты `app/`

### `01-install-deps.sh`

Устанавливает `build-essential`, `cmake`, `ninja-build`, `git`, `libpq-dev`, `qt6-base-dev`, `qt6-httpserver-dev`, `qt6-websockets-dev`, `libqt6sql6-psql`, `postgresql-client`, `openssl`.

### `02-create-app-user.sh`

- Создаёт системного `appuser` с домашней `/opt/auctionhub`, shell `nologin`, без sudo.
- Создаёт `/opt/auctionhub`, если нет.

### `03-setup-env.sh`

- Спрашивает пароль `dbuser` (или берёт из `DB_PASSWORD`).
- Генерирует `JWT_SECRET` через `openssl rand -hex 32`.
- Создаёт `/opt/auctionhub/app/.env` с реальными значениями.
- `chown root:appuser`, `chmod 640`.

### `04-clone-and-build.sh`

- Клонирует репозиторий в `/opt/auctionhub/app` (или `git pull`, если уже клонирован).
- Собирает приложение через `make setup && make build`.
- Проверяет, что `build/auctionhub_server` существует.

### `05-apply-schema.sh`

- Читает `.env`, берёт `AUCTIONHUB_DB_*`.
- Применяет `src/db/schema.sql` на db-сервере через `psql -h <DB_IP>`.

### `06-install-service.sh`

- Копирует `auctionhub.service` в `/etc/systemd/system/`.
- `systemctl daemon-reload`, `enable`, `restart`.

### `07-firewall.sh`

- Устанавливает UFW.
- Разрешает SSH (22) и HTTP API (8080/tcp) для всех.

---

## Порядок развёртывания

### На db-сервере

```bash
cd ~/DevOpsT3
sudo bash deploy/install.sh --db
```

Скрипт спросит пароль `dbuser`. Запомнить его — он понадобится на app-сервере.

Проверка:

```bash
sudo -u postgres psql -c "\du"      # dbuser должен быть в списке
sudo -u postgres psql -c "\l"       # auctionhub должна быть в списке
sudo ufw status                     # 5432/tcp ALLOW 192.168.56.101
```

### На app-сервере

```bash
cd ~/DevOpsT3
sudo bash deploy/install.sh --app
```

Скрипт спросит пароль `dbuser` — ввести **тот же**, что на db-сервере.

Проверка:

```bash
sudo systemctl status auctionhub                 # active (running)
curl http://192.168.56.101:8080/health           # {"database":"ok",...}
sudo ufw status                                  # 8080/tcp ALLOW Anywhere
```

С хоста:

```bash
curl http://192.168.56.101:8080/health
```

---

## Обновление приложения

При изменении кода в репозитории:

```bash
# на app-сервере
sudo -u appuser git -C /opt/auctionhub/app pull
sudo -u appuser bash -c 'cd /opt/auctionhub/app && make build'
sudo systemctl restart auctionhub
```

---

## Переменные и секреты

- **`deploy.conf`** — общие параметры, без секретов. В репозитории.
- **`/opt/auctionhub/app/.env`** — реальные значения для приложения. Создаётся `03-setup-env.sh`. В репозиторий не попадает.
- **Пароль `dbuser`** — вводится интерактивно дважды: при создании пользователя на db-сервере и при создании `.env` на app-сервере.

---

## Требования

- Приложение работает от `appuser`, не от `root`.
- `appuser` без sudo, без входа в систему.
- `dbuser` — SQL-пользователь с правами только на БД `auctionhub`.
- Root-логин по SSH запрещён.
- Пароли отключены, вход только по ключам.
- UFW: приложение — 8080 для всех, БД — 5432 только с app-IP.
- Секреты не в репозитории.
