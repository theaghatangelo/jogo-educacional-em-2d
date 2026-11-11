#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

// CONFIGURACOES
#define LARGURA_TELA 1024
#define ALTURA_TELA 800
#define DISTANCIA_PERTO 15
#define NUM_LOCAIS 3

// SPRITE SHEET
#define TAM_SPRITE 200
#define ESCALA_SPRITE 0.5f
#define TICKS_POR_PASSO 8

#define COL_ESQUERDA 0
#define COL_DIREITA 1
#define COL_FRENTE 2
#define COL_COSTAS 3

#define LIN_PARADO 0

// TELA INICIAL
#define TITULO_LARGURA 1920
#define TITULO_ALTURA 1080

// FAZENDA DE ERVILHAS
#define NUM_ERVILHAS 8
#define FAZ_X 256
#define FAZ_Y 140
#define ERVILHA_TAM 100

// ESTADOS DO JOGO
typedef enum {
    ESTADO_TELA_INICIAL,
    ESTADO_FASE_1
} Estado;

// LOCAIS DO MAPA
typedef struct {
    float x1, y1, x2, y2;
    const char* nome;
    const char* descricao;
} Local;

// AREAS CLICAVEIS
typedef struct {
    float x1, y1, x2, y2;
} Area;

// ERVILHAS
typedef struct {
    ALLEGRO_BITMAP* imagem;
    float x, y;
    int dominante;
    int coletada;
} Ervilha;

// VERIFICA SE O CLIQUE ESTA DENTRO DE UMA AREA
static int dentro(Area a, float px, float py) {
    return px >= a.x1 && px <= a.x2 &&
        py >= a.y1 && py <= a.y2;
}

int main() {

    // INICIALIZACAO DO ALLEGRO
    al_init();
    al_init_font_addon();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();

    ALLEGRO_DISPLAY* display =
        al_create_display(LARGURA_TELA, ALTURA_TELA);

    al_set_window_title(display, "PixelGENETICS");

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

    // IMAGENS
    ALLEGRO_BITMAP* titulo = al_load_bitmap("tela_inicial.png");
    if (!titulo) {
        printf("Erro: nao encontrei tela_inicial.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* sheet_pedro = al_load_bitmap("pedro_spritesheet.png");
    if (!sheet_pedro) {
        printf("Erro: nao encontrei pedro_spritesheet.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* sheet_amanda = al_load_bitmap("amanda_spritesheet.png");
    if (!sheet_amanda) {
        printf("Erro: nao encontrei amanda_spritesheet.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* fundo = al_load_bitmap("tela_fundo.png");
    if (!fundo) {
        printf("Erro: nao encontrei tela_fundo.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* seta = al_load_bitmap("seta.png");
    if (!seta) {
        printf("Erro: nao encontrei seta.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* fundo_fazenda = al_load_bitmap("fundo_fazenda.png");
    if (!fundo_fazenda) {
        printf("Erro: nao encontrei fundo_fazenda.png\n");
        return 1;
    }

    // IMAGEM DO LIVRO DE MENDEL
    ALLEGRO_BITMAP* livro_mendel = al_load_bitmap("livro_mendel.png");
    if (!livro_mendel) {
        printf("Erro: nao encontrei livro_mendel.png\n");
        return 1;
    }

    // ESTADO DO JOGO
    Estado estado = ESTADO_TELA_INICIAL;

    // AJUSTE DA IMAGEM DA TELA INICIAL
    float esc_titulo = (float)ALTURA_TELA / TITULO_ALTURA;
    float recorte_w = LARGURA_TELA / esc_titulo;
    float recorte_x = (TITULO_LARGURA - recorte_w) / 2;

    // OPCOES DE PERSONAGEM NA TELA INICIAL
    Area opcoes[2] = {
        {640, 300, 950, 745},
        {950, 300, 1260, 745}
    };

    int selecao = -1;

    float seta_x[2] = { 806, 1094 };
    float seta_y = 748;
    float seta_tam = 48;

    // LOCAIS DO MAPA
    Local locais[NUM_LOCAIS] = {
    {242, 232, 505, 394},
    {693, 265, 889, 361},
    {607, 467, 845, 693}
    };

    // PERSONAGEM
    ALLEGRO_BITMAP* sheet = NULL;

    float largura = TAM_SPRITE * ESCALA_SPRITE;
    float altura = TAM_SPRITE * ESCALA_SPRITE;

    float x = 480;
    float y = 380;
    float velocidade = 4;

    // ANIMACAO
    int direcao = COL_FRENTE;
    int andando = 0;
    int contador = 0;
    int passo = 0;

    int ciclo[4] = { 1, 0, 2, 0 };

    // INTERACAO COM LOCAIS
    int local_perto = -1;
    int popup_aberto = -1;


    // PAGINAS DO LIVRO DE MENDEL
    int pagina_mendel = 0;

    const char* textos_mendel[3] = {

        // PAGINA 1
        "Mendel cruzou ervilhas de linhagens puras com "
        "caracteristicas diferentes e observou como elas "
        "eram transmitidas entre geracoes.\n\n"
        "Suas descobertas foram:\n\n"
        "- Dominante: aparece na primeira geracao e esconde a recessiva.\n\n"
        "- Recessiva: fica escondida na primeira geracao, mas reaparece na segunda.\n\n"
        "- Proporcao: aproximadamente 3 plantas dominantes para cada 1 recessiva (3:1).\n\n"
        "- Independencia: uma caracteristica e herdada independentemente de outra.\n\n"
        "Mendel demonstrou que as caracteristicas sao transmitidas entre geracoes seguindo padroes de hereditariedade.",

        // PAGINA 2
        "Um dos principais trabalhos realizados por Gregor Mendel baseava-se no cruzamento de ervilhas a fim de compreender melhor os mecanismos de hereditariedade.\n\n"
        "As ervilhas foram o objeto de estudo ideal, uma vez que essas plantas possuem tempo de geracao curto, geram grande numero de descendentes por meio de cada cruzamento e apresentam uma serie de caracteristicas que podem ser analisadas.",

        // PAGINA 3
        "Em um dos seus experimentos, Mendel cruzou plantas puras de sementes lisas com plantas puras de sementes rugosas.\n\n"
        "Ele esperava obter individuos que fossem uma mistura entre as caracteristicas lisas e rugosas.\n\n"
        "Entretanto, observou apenas sementes lisas. Esses individuos foram chamados de hibridos, por serem resultado do cruzamento de plantas com caracteristicas diferentes.\n\n"
        "Mendel tambem realizou outros testes, como os famosos experimentos com ervilhas amarelas e verdes."
    };

    // ERVILHAS DA FAZENDA
    const char* arq_ervilha[NUM_ERVILHAS] = {
        "ervilha_1.png", "ervilha_3.png",
        "ervilha_2.png", "ervilha_6.png",
        "ervilha_5.png", "ervilha_7.png",
        "ervilha_4.png", "ervilha_8.png"
    };

    int dom_ervilha[NUM_ERVILHAS] = {
        1, 0, 0, 1, 0, 1, 1, 0
    };

    Ervilha ervilhas[NUM_ERVILHAS];

    for (int i = 0; i < NUM_ERVILHAS; i++) {

        ervilhas[i].imagem = al_load_bitmap(arq_ervilha[i]);

        if (!ervilhas[i].imagem) {
            printf("Erro: nao encontrei %s\n", arq_ervilha[i]);
            return 1;
        }

        int coluna = i % 4;
        int faixa = i / 4;

        ervilhas[i].x = FAZ_X + 21 + coluna * 123;
        ervilhas[i].y = FAZ_Y + (faixa == 0 ? 88 : 224);

        ervilhas[i].dominante = dom_ervilha[i];
        ervilhas[i].coletada = 0;
    }

    int ervilha_selecionada = -1;

    // FILA DE EVENTOS
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();

    al_register_event_source(event_queue,
        al_get_display_event_source(display));

    al_register_event_source(event_queue,
        al_get_timer_event_source(timer));

    al_register_event_source(event_queue,
        al_get_keyboard_event_source());

    al_register_event_source(event_queue,
        al_get_mouse_event_source());

    al_start_timer(timer);

    // LOOP PRINCIPAL
    while (1) {

        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

        // TELA INICIAL
        if (estado == ESTADO_TELA_INICIAL) {

            int escolhido = -1;

            // MOUSE SOBRE OS PERSONAGENS
            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {

                float mx = event.mouse.x / esc_titulo + recorte_x;
                float my = event.mouse.y / esc_titulo;

                selecao = -1;

                for (int i = 0; i < 2; i++) {
                    if (dentro(opcoes[i], mx, my)) {
                        selecao = i;
                    }
                }
            }

            // CLIQUE PARA ESCOLHER
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
                event.mouse.button == 1) {

                float mx = event.mouse.x / esc_titulo + recorte_x;
                float my = event.mouse.y / esc_titulo;

                for (int i = 0; i < 2; i++) {
                    if (dentro(opcoes[i], mx, my)) {
                        escolhido = i;
                    }
                }
            }

            // ESCOLHA PELO TECLADO
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {

                if (event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
                    selecao = 0;
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
                    selecao = 1;
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_ENTER &&
                    selecao != -1) {
                    escolhido = selecao;
                }
            }

            // CARREGA O PERSONAGEM ESCOLHIDO
            if (escolhido != -1) {

                if (escolhido == 0) {
                    sheet = sheet_pedro;
                }
                else {
                    sheet = sheet_amanda;
                }

                estado = ESTADO_FASE_1;
            }

            // DESENHO DA TELA INICIAL
            if (event.type == ALLEGRO_EVENT_TIMER &&
                estado == ESTADO_TELA_INICIAL) {

                al_draw_scaled_bitmap(
                    titulo,
                    recorte_x, 0,
                    recorte_w, TITULO_ALTURA,
                    0, 0,
                    LARGURA_TELA, ALTURA_TELA,
                    0
                );

                if (selecao != -1) {

                    float cx = (seta_x[selecao] - recorte_x) * esc_titulo;
                    float topo = seta_y * esc_titulo;

                    al_draw_scaled_bitmap(
                        seta,
                        0, 0,
                        al_get_bitmap_width(seta),
                        al_get_bitmap_height(seta),
                        cx - seta_tam / 2,
                        topo,
                        seta_tam,
                        seta_tam,
                        0
                    );
                }

                al_flip_display();
            }
        }

        // FASE 1
        else if (estado == ESTADO_FASE_1) {

            // ABRIR E FECHAR POP-UP
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {

                if (event.keyboard.keycode == ALLEGRO_KEY_E) {

                    if (popup_aberto != -1) {
                        popup_aberto = -1;
                    }
                    else if (local_perto != -1) {
                        popup_aberto = local_perto;

                        // Sempre comeca pela primeira pagina ao abrir a Casa
                        if (popup_aberto == 2) {
                            pagina_mendel = 0;
                        }
                    }
                }

                // PASSAR PAGINA DO LIVRO COM A TECLA P
                if (event.keyboard.keycode == ALLEGRO_KEY_P &&
                    popup_aberto == 2) {

                    pagina_mendel++;

                    if (pagina_mendel >= 3) {
                        pagina_mendel = 0;
                    }
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    popup_aberto = -1;
                }
            }

            // CLIQUE NAS ERVILHAS
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
                event.mouse.button == 1 &&
                popup_aberto == 1) {

                for (int i = 0; i < NUM_ERVILHAS; i++) {

                    if (event.mouse.x >= ervilhas[i].x &&
                        event.mouse.x <= ervilhas[i].x + ERVILHA_TAM &&
                        event.mouse.y >= ervilhas[i].y &&
                        event.mouse.y <= ervilhas[i].y + ERVILHA_TAM) {

                        ervilhas[i].coletada = 1;
                        ervilha_selecionada = i;
                    }
                }
            }

            // ATUALIZACAO DO JOGO
            if (event.type == ALLEGRO_EVENT_TIMER) {

                andando = 0;

                // MOVIMENTACAO
                // MOVIMENTACAO
                if (popup_aberto == -1) {

                    ALLEGRO_KEYBOARD_STATE teclado;
                    al_get_keyboard_state(&teclado);

                    // ANDAR PARA CIMA (W ou SETA PARA CIMA)
                    if (al_key_down(&teclado, ALLEGRO_KEY_W) ||
                        al_key_down(&teclado, ALLEGRO_KEY_UP)) {

                        y -= velocidade;
                        direcao = COL_COSTAS;
                        andando = 1;
                    }

                    // ANDAR PARA BAIXO (S ou SETA PARA BAIXO)
                    if (al_key_down(&teclado, ALLEGRO_KEY_S) ||
                        al_key_down(&teclado, ALLEGRO_KEY_DOWN)) {

                        y += velocidade;
                        direcao = COL_FRENTE;
                        andando = 1;
                    }

                    // ANDAR PARA ESQUERDA (A ou SETA PARA ESQUERDA)
                    if (al_key_down(&teclado, ALLEGRO_KEY_A) ||
                        al_key_down(&teclado, ALLEGRO_KEY_LEFT)) {

                        x -= velocidade;
                        direcao = COL_ESQUERDA;
                        andando = 1;
                    }

                    // ANDAR PARA DIREITA (D ou SETA PARA DIREITA)
                    if (al_key_down(&teclado, ALLEGRO_KEY_D) ||
                        al_key_down(&teclado, ALLEGRO_KEY_RIGHT)) {

                        x += velocidade;
                        direcao = COL_DIREITA;
                        andando = 1;
                    }

                    // LIMITES DA TELA
                    if (x < 0) x = 0;
                    if (y < 0) y = 0;

                    if (x > LARGURA_TELA - largura) {
                        x = LARGURA_TELA - largura;
                    }

                    if (y > ALTURA_TELA - altura) {
                        y = ALTURA_TELA - altura;
                    }
                }

                // ANIMACAO
                if (andando) {

                    contador++;

                    if (contador >= TICKS_POR_PASSO) {
                        contador = 0;
                        passo = (passo + 1) % 4;
                    }
                }
                else {
                    contador = 0;
                    passo = 0;
                }

                // VERIFICA SE ESTA PERTO DE ALGUM LOCAL
                local_perto = -1;

                for (int i = 0; i < NUM_LOCAIS; i++) {

                    if (x + largura > locais[i].x1 - DISTANCIA_PERTO &&
                        x < locais[i].x2 + DISTANCIA_PERTO &&
                        y + altura > locais[i].y1 - DISTANCIA_PERTO &&
                        y < locais[i].y2 + DISTANCIA_PERTO) {

                        local_perto = i;
                    }
                }

                // DESENHO DO MAPA
                al_draw_bitmap(fundo, 0, 0, 0);



                // DESENHO DO PERSONAGEM
                int linha = andando ? ciclo[passo] : LIN_PARADO;

                al_draw_scaled_bitmap(
                    sheet,
                    direcao * TAM_SPRITE,
                    linha * TAM_SPRITE,
                    TAM_SPRITE,
                    TAM_SPRITE,
                    x,
                    y,
                    largura,
                    altura,
                    0
                );

                // DICA PARA ENTRAR NOS LOCAIS
                if (local_perto != -1 && popup_aberto == -1) {

                    al_draw_filled_rectangle(
                        262, 740, 762, 780,
                        al_map_rgba(0, 0, 0, 180)
                    );

                    al_draw_textf(
                        font,
                        al_map_rgb(255, 255, 255),
                        LARGURA_TELA / 2,
                        756,
                        ALLEGRO_ALIGN_CENTRE,
                        "Aperte E para entrar"
                    );
                }

                // POP-UP
                if (popup_aberto != -1) {

                    al_draw_filled_rectangle(
                        0, 0,
                        LARGURA_TELA, ALTURA_TELA,
                        al_map_rgba(0, 0, 0, 150)
                    );

                    // FAZENDA DE ERVILHAS
                    if (popup_aberto == 1) {

                        al_draw_bitmap(fundo_fazenda, FAZ_X, FAZ_Y, 0);

                        int total_coletadas = 0;

                        // DESENHA AS ERVILHAS
                        for (int i = 0; i < NUM_ERVILHAS; i++) {

                            if (ervilhas[i].coletada) {

                                total_coletadas++;

                                al_draw_tinted_bitmap(
                                    ervilhas[i].imagem,
                                    al_map_rgba(120, 120, 120, 255),
                                    ervilhas[i].x,
                                    ervilhas[i].y,
                                    0
                                );
                            }
                            else {

                                al_draw_bitmap(
                                    ervilhas[i].imagem,
                                    ervilhas[i].x,
                                    ervilhas[i].y,
                                    0
                                );
                            }
                        }

                        // RESULTADO DA ERVILHA CLICADA
                        if (ervilha_selecionada == -1) {

                            al_draw_text(
                                font,
                                al_map_rgb(255, 255, 255),
                                LARGURA_TELA / 2,
                                590,
                                ALLEGRO_ALIGN_CENTRE,
                                "Clique em uma ervilha!"
                            );
                        }
                        else if (ervilhas[ervilha_selecionada].dominante) {

                            al_draw_text(
                                font,
                                al_map_rgb(168, 230, 29),
                                LARGURA_TELA / 2,
                                590,
                                ALLEGRO_ALIGN_CENTRE,
                                "DOMINANTE"
                            );
                        }
                        else {

                            al_draw_text(
                                font,
                                al_map_rgb(255, 194, 14),
                                LARGURA_TELA / 2,
                                590,
                                ALLEGRO_ALIGN_CENTRE,
                                "RECESSIVA"
                            );
                        }

                        // CONTAGEM
                        char contagem[40];

                        snprintf(
                            contagem,
                            sizeof(contagem),
                            "Coletadas: %d/%d",
                            total_coletadas,
                            NUM_ERVILHAS
                        );

                        al_draw_text(
                            font,
                            al_map_rgb(255, 255, 255),
                            LARGURA_TELA / 2,
                            620,
                            ALLEGRO_ALIGN_CENTRE,
                            contagem
                        );

                        al_draw_text(
                            font,
                            al_map_rgb(255, 255, 255),
                            LARGURA_TELA / 2,
                            650,
                            ALLEGRO_ALIGN_CENTRE,
                            "Aperte E ou ESC para fechar"
                        );
                    }

                    // CASA - LIVRO DE MENDEL
                    else if (popup_aberto == 2) {

                        // DESENHA A IMAGEM DO LIVRO NA TELA
                        al_draw_scaled_bitmap(
                            livro_mendel,
                            0, 0,
                            al_get_bitmap_width(livro_mendel),
                            al_get_bitmap_height(livro_mendel),
                            0, 0,
                            LARGURA_TELA,
                            ALTURA_TELA,
                            0
                        );

                        // DESENHA O TEXTO SOBRE AS DUAS PAGINAS BRANCAS
                        al_draw_multiline_text(
                            font,
                            al_map_rgb(40, 40, 40),
                            130,                 // Posicao X do texto
                            450,                 // Posicao Y do texto
                            760,                 // Largura maxima do texto
                            15,                  // Espacamento entre linhas
                            ALLEGRO_ALIGN_LEFT,
                            textos_mendel[pagina_mendel]
                        );
                    }

                    // OUTROS LOCAIS (LABORATORIO)
                    else {

                        al_draw_filled_rectangle(
                            212, 250, 812, 550,
                            al_map_rgb(255, 255, 255)
                        );

                        al_draw_rectangle(
                            212, 250, 812, 550,
                            al_map_rgb(3, 169, 244),
                            4
                        );
                    }
                }

                al_flip_display();
            }
        }
    }

    // LIBERAR MEMORIA
    al_destroy_bitmap(titulo);
    al_destroy_bitmap(seta);
    al_destroy_bitmap(sheet_pedro);
    al_destroy_bitmap(sheet_amanda);
    al_destroy_bitmap(fundo);
    al_destroy_bitmap(fundo_fazenda);
    al_destroy_bitmap(livro_mendel);

    for (int i = 0; i < NUM_ERVILHAS; i++) {
        al_destroy_bitmap(ervilhas[i].imagem);
    }

    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}
