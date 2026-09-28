# Trabalho Prático — Computação Gráfica (Sistema Solar Simplificado)

Disciplina ECOI24 — Computação Gráfica e Processamento Digital de Imagens
Professor: Dr. André Ribeiro de Brito

## Dependências (Ubuntu/Pop!_OS/Debian)
```
sudo apt install build-essential freeglut3-dev
```

## Compilar
```
gcc src/sistema_solar.c -o sistema_solar -lglut -lGL -lGLU -lm
```

## Executar
O programa precisa das texturas `earth.bmp` e `moon.bmp` na mesma pasta do executável:
```
cp src/earth.bmp src/moon.bmp .
./sistema_solar
```

## Controles
| Tecla | Ação |
|---|---|
| A / D | Gira a câmera ao redor da cena |
| W / S | Inclina a câmera (pitch) |
| + / - | Zoom |
| P | Pausa/retoma a animação |
| Esc | Sai do programa |