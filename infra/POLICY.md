# Требования к инфраструктурному коду

Нарушение любого пункта блокирует слияние (проверки в `.github/workflows/infra-cd.yml`).

| # | Требование | Чем обеспечивается |
|---|-----------|--------------------|
| 1 | Нет лишних открытых портов: ufw `deny incoming` по умолчанию, открыт только SSH и порт сервиса, и только для нужных адресов | роль `firewall`; `policy-check.sh` запрещает правила `from: any`; `verify.yml` после каждого запуска сверяет реально слушающие порты со списком `verify_allowed_ports` |
| 2 | Порты контейнеров публикуются на конкретный IP узла, не на `0.0.0.0` | `policy-check.sh` (шаблоны compose) |
| 3 | Минимальные права: сервисная учётная запись без shell и sudo; секретные файлы `0600`; конфигурации `0640/0644`; пользователи не добавляются в группу `docker` | роли `common`, `app`, `db`; `policy-check.sh` запрещает world-writable режимы |
| 4 | SSH: только ключи, без root, `AllowUsers` | шаблон `sshd-hardening.conf.j2`, проверка `sshd -t` перед установкой |
| 5 | Нет необъяснимых shell-команд: только модули Ansible. `command` допустим только с `changed_when` и комментарием, зачем он нужен | `ansible-lint` (profile `production`), `policy-check.sh` |
| 6 | Повторяемость: версии инструментов и коллекций закреплены; образ PostgreSQL закреплён по digest; production разворачивается с тега/коммита; `state: latest` запрещён | `requirements.txt`, `requirements.yml`, `group_vars`, `policy-check.sh` |
| 7 | Идемпотентность: повторный запуск даёт `changed=0` | шаг «Idempotency check» в CD |
| 8 | Секреты только в `vault.yml`, зашифрованном `ansible-vault`; в `group_vars` — только ссылки на `vault_*`; задачи с секретами `no_log: true` | `policy-check.sh` (проверка заголовка `$ANSIBLE_VAULT`, поиск открытых значений), gitleaks в основном CI |
| 9 | Критичные файлы меняются только после согласования владельца | `.github/CODEOWNERS` + защита ветки `main`; environment `production` с обязательным согласующим |

Ослабление правил (`.ansible-lint`, `policy-check.sh`, этот файл) — только отдельным PR с обоснованием.
