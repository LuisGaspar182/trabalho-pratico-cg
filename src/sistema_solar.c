/*
 * Trabalho Pratico - Computacao Grafica (ECOI24)
 * Sistema Solar Simplificado (Sol + Terra + Lua)
 *
 * Texturas: Sol (ruido procedural), Terra e Lua (fotos reais NASA,
 * carregadas de arquivos .bmp que devem estar na mesma pasta do executavel).
 * Interacao: teclado controla camera (a/d/w/s), zoom (+/-) e pausa (p).
 *
 * Compilar: gcc sistema_solar.c -o sistema_solar -lglut -lGL -lGLU
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>
#include <GL/freeglut.h>

#define DISTANCIA_ORBITA_TERRA 4.5f
#define DISTANCIA_ORBITA_LUA 1.0f
#define TAM_TEXTURA_SOL 128
float anguloOrbitaTerra = 0.0f;
float anguloOrbitaLua = 0.0f;
float tempoAnimacao = 0.0f;
int animacaoPausada = 0;
GLuint idTexturaSol;
GLuint idTexturaTerra;
GLuint idTexturaLua;
GLUquadric *quadrica;

/* Camera: orbita ao redor da origem, controlada pelo teclado. */
float cameraYaw = 0.0f;
float cameraPitch = 18.0f;
float cameraDistancia = 12.0f;

void inicializar(void);
int carregarBMP(const char *caminho, unsigned char **dados, int *largura, int *altura);
void gerarTexturaSol(void);
void posicionarCamera(void);
void desenhar(void);
void redimensionar(int largura, int altura);
void teclado(unsigned char tecla, int x, int y);
void temporizador(int valor);

/*
 * Le um arquivo .bmp de 24 bits sem compressao e devolve os pixels em RGB,
 * com as linhas ja na ordem topo-para-baixo (o BMP guarda de baixo pra
 * cima e em BGR, entao aqui e feita a conversao).
 * Retorna 1 se deu certo, 0 se falhou.
 */
int carregarBMP(const char *caminho, unsigned char **dados, int *largura, int *altura)
{
    FILE *arquivo = fopen(caminho, "rb");
    if (!arquivo) {
        return 0;
    }

    unsigned char cabecalho[54];
    if (fread(cabecalho, 1, 54, arquivo) != 54 || cabecalho[0] != 'B' || cabecalho[1] != 'M') {
        fclose(arquivo);
        return 0;
    }

    int deslocamentoDados, largItem, altItem;
    short bitsPorPixel;
    memcpy(&deslocamentoDados, &cabecalho[10], 4);
    memcpy(&largItem, &cabecalho[18], 4);
    memcpy(&altItem, &cabecalho[22], 4);
    memcpy(&bitsPorPixel, &cabecalho[28], 2);

    if (bitsPorPixel != 24) {
        fclose(arquivo);
        return 0;
    }

    /* Cada linha do BMP e alinhada em multiplos de 4 bytes. */
    int bytesPorLinha = (largItem * 3 + 3) & ~3;
    unsigned char *linhaBGR = malloc(bytesPorLinha);
    unsigned char *pixelsRGB = malloc((size_t)largItem * altItem * 3);

    fseek(arquivo, deslocamentoDados, SEEK_SET);

    for (int y = 0; y < altItem; y++) {
        fread(linhaBGR, 1, bytesPorLinha, arquivo);

        /* Inverte a ordem das linhas (BMP e de baixo pra cima) e troca BGR por RGB. */
        int linhaDestino = altItem - 1 - y;
        for (int x = 0; x < largItem; x++) {
            int i = (linhaDestino * largItem + x) * 3;
            pixelsRGB[i]     = linhaBGR[x * 3 + 2]; /* R */
            pixelsRGB[i + 1] = linhaBGR[x * 3 + 1]; /* G */
            pixelsRGB[i + 2] = linhaBGR[x * 3];     /* B */
        }
    }

    free(linhaBGR);
    fclose(arquivo);

    *dados = pixelsRGB;
    *largura = largItem;
    *altura = altItem;
    return 1;
}

/*
 * Textura do Sol: ruido gerado por soma de senos (Aula 12 - mapa de
 * ruido, mesmo principio usado la para madeira e marmore). Nao usa foto
 * real porque o Sol e auto-luminoso, nao teria sentido "fotografar" ele.
 */
void gerarTexturaSol(void)
{
    unsigned char dados[TAM_TEXTURA_SOL * TAM_TEXTURA_SOL * 3];

    for (int y = 0; y < TAM_TEXTURA_SOL; y++) {
        for (int x = 0; x < TAM_TEXTURA_SOL; x++) {
            float ruido = sinf(x * 0.25f) * cosf(y * 0.2f) + sinf((x + y) * 0.1f);
            float t = (ruido + 2.0f) / 4.0f; /* normaliza de [-2,2] para [0,1] */

            int i = (y * TAM_TEXTURA_SOL + x) * 3;
            dados[i]     = 255;
            dados[i + 1] = (unsigned char)(140 + t * 100);
            dados[i + 2] = (unsigned char)(t * 60);
        }
    }

    glGenTextures(1, &idTexturaSol);
    glBindTexture(GL_TEXTURE_2D, idTexturaSol);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TAM_TEXTURA_SOL, TAM_TEXTURA_SOL, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, dados);
}

void inicializar(void)
{
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f); /* fundo escuro (espaco) */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    gerarTexturaSol();

    /* Transparencia para o halo pulsante ao redor do Sol (Aula 9 -
       Modelo RGB / Cores em OpenGL - glColor4f + GL_BLEND). */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Carrega a foto da Terra (NASA Blue Marble) e envia para a GPU. */
    unsigned char *pixels;
    int largura, altura;
    if (carregarBMP("earth.bmp", &pixels, &largura, &altura)) {
        glGenTextures(1, &idTexturaTerra);
        glBindTexture(GL_TEXTURE_2D, idTexturaTerra);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, largura, altura, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, pixels);
        free(pixels);
    } else {
        fprintf(stderr, "Aviso: nao foi possivel abrir earth.bmp (deve estar na mesma pasta do executavel)\n");
    }

    /* Carrega a foto da Lua (NASA) e envia para a GPU. */
    if (carregarBMP("moon.bmp", &pixels, &largura, &altura)) {
        glGenTextures(1, &idTexturaLua);
        glBindTexture(GL_TEXTURE_2D, idTexturaLua);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, largura, altura, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, pixels);
        free(pixels);
    } else {
        fprintf(stderr, "Aviso: nao foi possivel abrir moon.bmp (deve estar na mesma pasta do executavel)\n");
    }

    /* gluSphere precisa de um quadric para gerar coordenadas de textura
       (Aula 8 - gluSphere como superficie de revolucao via GLU). */
    quadrica = gluNewQuadric();
    gluQuadricTexture(quadrica, GL_TRUE);
    gluQuadricNormals(quadrica, GLU_SMOOTH);
}

/* Camera em orbita ao redor da origem (Aula 5 - gluLookAt), com
   posicao calculada por coordenadas esfericas a partir de yaw/pitch/distancia. */
void posicionarCamera(void)
{
    const float radianos = 3.14159265f / 180.0f;
    float yaw = cameraYaw * radianos;
    float pitch = cameraPitch * radianos;

    float x = cameraDistancia * cosf(pitch) * sinf(yaw);
    float y = cameraDistancia * sinf(pitch);
    float z = cameraDistancia * cosf(pitch) * cosf(yaw);

    gluLookAt(
        x, y, z,
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    );
}

void desenhar(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    posicionarCamera();

    /* Sol: tamanho fixo, com a textura de ruido. */
    glPushMatrix();
        glColor3f(1.0f, 1.0f, 1.0f);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, idTexturaSol);
        gluSphere(quadrica, 1.5, 32, 32);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    /* Terra orbitando: primeiro roda o eixo, depois desloca (a ordem importa). */
    glPushMatrix();
        glRotatef(anguloOrbitaTerra, 0.0f, 1.0f, 0.0f);
        glTranslatef(DISTANCIA_ORBITA_TERRA, 0.0f, 0.0f);

        /* Terra com textura (glTexCoord e gerado pelo proprio gluSphere,
           por causa do gluQuadricTexture ativado em inicializar()). */
        glPushMatrix();
            /* gluSphere posiciona os polos no eixo Z; gira 90 em X para
               alinhar com Y (o "para cima" da nossa camera). */
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            glColor3f(1.0f, 1.0f, 1.0f); /* branco, para nao tingir a textura */
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, idTexturaTerra);
            gluSphere(quadrica, 0.5, 24, 24);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();

        /* Lua: orbita a Terra. Fica dentro do push/pop da Terra, entao
           herda a posicao dela (transformacao aninhada). */
        glPushMatrix();
            glRotatef(anguloOrbitaLua, 0.0f, 1.0f, 0.0f);
            glTranslatef(DISTANCIA_ORBITA_LUA, 0.0f, 0.0f);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f); /* mesmo ajuste de polo da Terra */
            glColor3f(1.0f, 1.0f, 1.0f);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, idTexturaLua);
            gluSphere(quadrica, 0.15, 16, 16);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    glPopMatrix();

    /* Halo: esfera um pouco maior e semitransparente ao redor do Sol,
       que pulsa de tamanho (Aula 9 - GL_BLEND para a transparencia).
       Fica coladinho no Sol (raio pequeno) para nunca alcançar a
       distancia da orbita da Terra, evitando ambiguidade de profundidade
       entre um objeto transparente e um objeto solido distante. */
    glPushMatrix();
        float escalaHalo = 1.0f + 0.1f * sinf(tempoAnimacao);
        glScalef(escalaHalo, escalaHalo, escalaHalo);
        glDepthMask(GL_FALSE); /* nao escreve no depth buffer, so "brilha" por cima */
        glColor4f(1.0f, 0.85f, 0.3f, 0.35f); /* amarelo, 35% opaco */
        glutSolidSphere(1.65, 24, 24);
        glDepthMask(GL_TRUE);
    glPopMatrix();

    glutSwapBuffers();
}

void redimensionar(int largura, int altura)
{
    if (altura == 0) altura = 1;

    glViewport(0, 0, largura, altura);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)largura / altura, 0.5, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

/*
 * Teclado:
 * a/d - gira a camera ao redor da cena (yaw)
 * w/s - inclina a camera (pitch)
 * +/- - zoom (aproxima/afasta)
 * p   - pausa/retoma a animacao
 * Esc - sai do programa
 */
void teclado(unsigned char tecla, int x, int y)
{
    (void)x;
    (void)y;

    switch (tecla) {
        case 27: /* Esc */
            exit(EXIT_SUCCESS);
            break;

        case 'a': case 'A':
            cameraYaw -= 3.0f;
            break;
        case 'd': case 'D':
            cameraYaw += 3.0f;
            break;

        case 'w': case 'W':
            cameraPitch += 3.0f;
            if (cameraPitch > 85.0f) cameraPitch = 85.0f;
            break;
        case 's': case 'S':
            cameraPitch -= 3.0f;
            if (cameraPitch < -85.0f) cameraPitch = -85.0f;
            break;

        case '+':
            cameraDistancia -= 0.5f;
            if (cameraDistancia < 4.0f) cameraDistancia = 4.0f;
            break;
        case '-':
            cameraDistancia += 0.5f;
            if (cameraDistancia > 30.0f) cameraDistancia = 30.0f;
            break;

        case 'p': case 'P':
            animacaoPausada = !animacaoPausada;
            break;
    }

    glutPostRedisplay();
}

/* Roda a cada quadro (~60 fps): avanca a orbita (se nao estiver pausado)
   e se reagenda. */
void temporizador(int valor)
{
    (void)valor;

    if (!animacaoPausada) {
        tempoAnimacao += 0.05f;

        anguloOrbitaTerra += 0.6f;
        if (anguloOrbitaTerra > 360.0f) anguloOrbitaTerra -= 360.0f;

        anguloOrbitaLua += 2.4f; /* Lua orbita mais rapido que a Terra */
        if (anguloOrbitaLua > 360.0f) anguloOrbitaLua -= 360.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, temporizador, 0);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Trabalho Pratico - Sistema Solar Simplificado");

    inicializar();
    glutDisplayFunc(desenhar);
    glutReshapeFunc(redimensionar);
    glutKeyboardFunc(teclado);
    glutTimerFunc(16, temporizador, 0);
    glutMainLoop();

    return EXIT_SUCCESS;
}