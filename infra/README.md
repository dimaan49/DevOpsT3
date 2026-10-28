# Infrastructure (Ansible)

Подготовка двух Ubuntu Server (arm64) для AuctionHub: пользователи, SSH, пакеты, Docker,
каталоги и права, firewall, конфигурация из шаблонов, запуск контейнеров и проверка доступности.

## Узлы

| Узел | Группа | Назначение | Что работает |
|------|--------|-----------|--------------|
| `auction-app` | `app` | сервер приложения | Docker-контейнер `auctionhub` (HTTP API) |
| `auction-db`  | `db`  | сервер БД | Docker-контейнер PostgreSQL |

## Где какие параметры

| Уровень | Файл | Примеры |
|---------|------|---------|
| Общие | `group_vars/all.yml`, `roles/*/defaults/` | имя проекта, сервисный пользователь, образ PostgreSQL, SSH-порт |
| Окружение | `inventories/<env>/group_vars/` | порты, уровень логов, лимиты, ссылка на версию кода |
| Узел | `inventories/<env>/host_vars/<host>.yml` | IP-адрес узла |
| Секреты | `inventories/<env>/group_vars/all/vault.yml` (зашифрован) | пароли БД, JWT-секрет |

Окружения: `staging` (порты 8081/5433, ветка `main`) и `production` (порты 8080/5432, закреплённый тег).
Оба разворачиваются на одних и тех же двух ВМ в разные каталоги `/opt/auctionhub-<env>` и разные compose-проекты.

## Запуск

```bash
cd infra
python3 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
ansible-galaxy collection install -r requirements.yml

export ANSIBLE_VAULT_PASSWORD_FILE=~/.vault/staging.pass   # файл вне репозитория
ansible-playbook -i inventories/staging/hosts.yml site.yml --ask-become-pass   # при необходимости
ansible-playbook -i inventories/staging/hosts.yml verify.yml                    # только проверки
```

Секреты: см. `inventories/*/group_vars/all/vault.yml.example`. Пароли генерируйте через `openssl rand -hex 32`
(без спецсимволов, т.к. значения попадают в `.env` для Compose).

Требования к коду — в [POLICY.md](POLICY.md). Локальная проверка: `bash policy-check.sh && yamllint . && ansible-lint`.
