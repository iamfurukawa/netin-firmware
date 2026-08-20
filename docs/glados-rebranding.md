# Rebranding público: GLaDOS

Os repositórios, containers, banco, tópicos MQTT e credenciais internas
permanecem com o prefixo técnico `netin`. Somente o nome exibido e os hosts
públicos são alterados.

| Uso | Host público |
| --- | --- |
| PWA | `glados.13997906387.xyz` |
| API | `glados-server.13997906387.xyz` |
| MQTT sobre WSS | `glados-mqtt.13997906387.xyz` |

## Raspberry

Antes de instalar o firmware com os novos hosts, inclua os três mapeamentos em
`/srv/nginx/nginx.conf`, preservando os hosts `netin-*` antigos durante a
transição:

```nginx
glados.13997906387.xyz        netin-web:80;
glados-server.13997906387.xyz netin-server:3000;
glados-mqtt.13997906387.xyz   mosquitto:9001;
```

O bloco específico de MQTT/WebSocket deve aceitar também
`glados-mqtt.13997906387.xyz` e manter o encaminhamento para `mosquitto:9001`
no caminho `/mqtt`. Valide e recarregue:

```bash
cd /srv/nginx
docker exec nginx nginx -t
docker exec nginx nginx -s reload
```

O Cloudflare Tunnel já usa o host curinga. Confirme apenas que o DNS curinga
`*.13997906387.xyz` continua apontando ao tunnel.

## Variáveis de produção

Em `/srv/netin-server/.env.production`, altere:

```dotenv
CORS_ORIGIN=https://glados.13997906387.xyz
PUBLIC_API_URL=https://glados-server.13997906387.xyz
```

O deploy do servidor aplica essas variáveis ao recriar a API. O deploy da PWA
já recebe `VITE_API_BASE_URL=https://glados-server.13997906387.xyz` pelo
Compose versionado.

## Verificação

```bash
curl -i https://glados-server.13997906387.xyz/health
curl -i https://glados.13997906387.xyz/
```

Depois de os dois hosts responderem, o firmware pode ser gravado. Dispositivos
já pareados mantêm a mesma identidade e credencial; não é necessário parear
novamente.
