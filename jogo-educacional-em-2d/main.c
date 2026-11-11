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

// DESAFIO DO LABORATORIO
#define TEMPO_DESAFIO 45.0
// FASE 2
#define NUM_CASAS 5
#define SETA_FASE2_TAM 64
#define SETA_FASE2_X (LARGURA_TELA - SETA_FASE2_TAM - 8)
#define SETA_FASE2_Y ((ALTURA_TELA - SETA_FASE2_TAM) / 2)

// ESTADOS DO JOGO
typedef enum {
    ESTADO_TELA_INICIAL,
    ESTADO_FASE_1,
    ESTADO_FASE_2
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

// DEFINE UMA MENSAGEM COM SEGURANCA
static void definir_mensagem(char destino[], int tamanho,
    const char* mensagem) {
    snprintf(destino, tamanho, "%s", mensagem);
}
// LE A TECLA 1, 2, 3 OU 4 (0 = OUTRA TECLA)
static int tecla_resposta(int tecla) {
    if (tecla == ALLEGRO_KEY_1 || tecla == ALLEGRO_KEY_PAD_1) return 1;
    if (tecla == ALLEGRO_KEY_2 || tecla == ALLEGRO_KEY_PAD_2) return 2;
    if (tecla == ALLEGRO_KEY_3 || tecla == ALLEGRO_KEY_PAD_3) return 3;
    if (tecla == ALLEGRO_KEY_4 || tecla == ALLEGRO_KEY_PAD_4) return 4;
    return 0;
}

// MOVIMENTA O PERSONAGEM (USADO NAS DUAS FASES)
static void mover_personagem(float* x, float* y, int* direcao,
    int* andando, float velocidade, float largura, float altura) {

    ALLEGRO_KEYBOARD_STATE teclado;
    al_get_keyboard_state(&teclado);

    if (al_key_down(&teclado, ALLEGRO_KEY_W) ||
        al_key_down(&teclado, ALLEGRO_KEY_UP)) {
        *y -= velocidade;
        *direcao = COL_COSTAS;
        *andando = 1;
    }

    if (al_key_down(&teclado, ALLEGRO_KEY_S) ||
        al_key_down(&teclado, ALLEGRO_KEY_DOWN)) {
        *y += velocidade;
        *direcao = COL_FRENTE;
        *andando = 1;
    }

    if (al_key_down(&teclado, ALLEGRO_KEY_A) ||
        al_key_down(&teclado, ALLEGRO_KEY_LEFT)) {
        *x -= velocidade;
        *direcao = COL_ESQUERDA;
        *andando = 1;
    }

    if (al_key_down(&teclado, ALLEGRO_KEY_D) ||
        al_key_down(&teclado, ALLEGRO_KEY_RIGHT)) {
        *x += velocidade;
        *direcao = COL_DIREITA;
        *andando = 1;
    }

    // LIMITES DA TELA
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*x > LARGURA_TELA - largura) *x = LARGURA_TELA - largura;
    if (*y > ALTURA_TELA - altura) *y = ALTURA_TELA - altura;
}

// ANIMACAO DOS PASSOS (USADA NAS DUAS FASES)
static void animar(int andando, int* contador, int* passo) {

    if (andando) {
        (*contador)++;

        if (*contador >= TICKS_POR_PASSO) {
            *contador = 0;
            *passo = (*passo + 1) % 4;
        }
    }
    else {
        *contador = 0;
        *passo = 0;
    }
}

// DESENHA O PERSONAGEM
static void desenhar_personagem(ALLEGRO_BITMAP* sheet, int direcao,
    int andando, int frame_andando, float x, float y,
    float largura, float altura) {

    int linha = andando ? frame_andando : LIN_PARADO;

    al_draw_scaled_bitmap(
        sheet,
        direcao * TAM_SPRITE, linha * TAM_SPRITE,
        TAM_SPRITE, TAM_SPRITE,
        x, y, largura, altura,
        0
    );
}

int main() {

    // INICIALIZACAO DO ALLEGRO
    if (!al_init()) {
        printf("Erro ao inicializar Allegro.\n");
        return 1;
    }

    al_init_font_addon();
    al_init_image_addon();
    al_init_primitives_addon();

    if (!al_install_keyboard()) {
        printf("Erro ao inicializar teclado.\n");
        return 1;
    }

    al_install_mouse();

    ALLEGRO_DISPLAY* display =
        al_create_display(LARGURA_TELA, ALTURA_TELA);

    if (!display) {
        printf("Erro ao criar a tela.\n");
        return 1;
    }

    al_set_window_title(display, "PixelGENETICS");

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

    if (!font || !timer) {
        printf("Erro ao criar fonte ou temporizador.\n");
        return 1;
    }

    // IMAGENS
    ALLEGRO_BITMAP* titulo = al_load_bitmap("tela_inicial.png");
    if (!titulo) {
        printf("Erro: nao encontrei tela_inicial.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* sheet_pedro =
        al_load_bitmap("pedro_spritesheet.png");

    if (!sheet_pedro) {
        printf("Erro: nao encontrei pedro_spritesheet.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* sheet_amanda =
        al_load_bitmap("amanda_spritesheet.png");

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
    ALLEGRO_BITMAP* fundo_fase2 = al_load_bitmap("fundo_fase_2.png");
    if (!fundo_fase2) {
        printf("Erro: nao encontrei fundo_fase_2.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* fundo_fazenda =
        al_load_bitmap("fundo_fazenda.png");

    if (!fundo_fazenda) {
        printf("Erro: nao encontrei fundo_fazenda.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* livro_mendel =
        al_load_bitmap("livro_mendel.png");

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

    // OPCOES DE PERSONAGEM
    Area opcoes[2] = {
        {640, 300, 950, 745},
        {950, 300, 1260, 745}
    };

    int selecao = -1;

    float seta_x[2] = { 806, 1094 };
    float seta_y = 748;
    float seta_tam = 48;

    // LOCAIS DO MAPA
    // 0 = LABORATORIO | 1 = FAZENDA | 2 = CASA
    Local locais[NUM_LOCAIS] = {
        {242, 232, 505, 394, "Laboratorio", "Teste de Mendel"},
        {693, 265, 889, 361, "Fazenda", "Ervilhas"},
        {607, 467, 845, 693, "Casa", "Livro de Mendel"}
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
        "Um dos principais trabalhos realizados por Gregor Mendel baseava-se no cruzamento de "
        "ervilhas a fim de compreender melhor os mecanismos de hereditariedade.\n\n"
        "As ervilhas foram o objeto de estudo ideal, uma vez que essas plantas possuem tempo de "
        "geracao curto, geram grande numero de descendentes por meio de cada cruzamento e "
        "apresentam uma serie de caracteristicas que podem ser analisadas.",

        // PAGINA 3
        "Em um dos seus experimentos, Mendel cruzou plantas puras de sementes lisas com plantas "
        "puras de sementes rugosas.\n\n"
        "Ele esperava obter individuos que fossem uma mistura entre as caracteristicas lisas e rugosas.\n\n"
        "Entretanto, observou apenas sementes lisas. Esses individuos foram chamados de hibridos, "
        "por serem resultado do cruzamento de plantas com caracteristicas diferentes.\n\n"
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

    // DESAFIO DO LABORATORIO
    int pergunta_mendel = 0;
    int desafio_iniciado = 0;
    int desafio_concluido = 0;

    double inicio_desafio = 0;
    int tempo_restante = 45;

    char mensagem_desafio[160] = "";

    const char* perguntas_mendel[3] = {

        "No cruzamento Aa x Aa, qual a proporcao fenotipica esperada?",

        "No cruzamento AA x aa, como serao os descendentes?",

        "No cruzamento aa x aa, como serao os descendentes?"
    };

    const char* alternativas_mendel[3][4] = {

        {
            "1 - 1:1",
            "2 - 3:1",
            "3 - 1:3",
            "4 - 100% recessivo"
        },

        {
            "1 - 100% Aa",
            "2 - 50% AA e 50% aa",
            "3 - 100% AA",
            "4 - 100% aa"
        },

        {
            "1 - 100% dominantes",
            "2 - 50% dominantes",
            "3 - 100% recessivos",
            "4 - 3:1"
        }
    };

    // RESPOSTAS CORRETAS
    // PERGUNTA 1 = ALTERNATIVA 2
    // PERGUNTA 2 = ALTERNATIVA 1
    // PERGUNTA 3 = ALTERNATIVA 3
    int respostas_corretas[3] = { 2, 1, 3 };
    // FASE 2
    int fase2_liberada = 0;

    // CASAS DA FASE 2
    // 0 = AMARELA | 1 = ROSA | 2 = VERDE | 3 = AZUL | 4 = VERMELHA
    Local casas[NUM_CASAS] = {
        {719, 50, 958, 275, "CASA AMARELA", "Daltonismo"},
        {29, 154, 268, 379, "CASA ROSA", "Tipo sanguineo ABO"},
        {377, 294, 616, 520, "CASA VERDE", "Fator Rh"},
        {639, 543, 878, 768, "CASA AZUL", "Albinismo"},
        {58, 564, 297, 789, "CASA VERMELHA", "Sexo do bebe"}
    };

    int popup_casa = -1;
    int casa_fechada = -1;
    int casa_ok[NUM_CASAS] = { 0, 0, 0, 0, 0 };

    char mensagem_casa[80] = "";

    const char* perguntas_casas[NUM_CASAS] = {

        "Um menino nasceu daltonico. O gene do daltonismo esta no "
        "cromossomo X. De quem ele herdou esse gene?",

        "Um pai de sangue tipo O e uma mae de sangue tipo AB "
        "tem um filho. Quais tipos sanguineos o filho pode ter?",

        "O pai e Rh positivo (Rr) e a mae e Rh negativo (rr). "
        "Qual a chance de o filho ser Rh negativo?",

        "Pai e mae tem pele normal (Aa), mas os dois carregam "
        "o gene do albinismo (a). Qual a chance de terem um filho albino?",

        "Quem determina o sexo biologico do bebe?"
    };

    const char* alternativas_casas[NUM_CASAS][4] = {
        { "1 - Do pai", "2 - Da mae",
          "3 - Do pai e da mae igualmente", "4 - Nao e herdado" },

        { "1 - Apenas O", "2 - Apenas AB",
          "3 - A ou B", "4 - AB ou O" },

        { "1 - 50%", "2 - 0%", "3 - 25%", "4 - 100%" },

        { "1 - 0%", "2 - 50%", "3 - 75%", "4 - 25%" },

        { "1 - O espermatozoide do pai (X ou Y)",
          "2 - O ovulo da mae (sempre X)",
          "3 - Os dois decidem igualmente",
          "4 - Depende so do ambiente" }
    };

    const char* explicacoes_casas[NUM_CASAS] = {
        "O gene do daltonismo fica no cromossomo X. O menino recebe "
        "o X da mae e o Y do pai, entao ele herdou o gene da mae.",

        "O (ii) x AB (IA IB): o filho recebe i de um e IA ou IB do "
        "outro. Ou seja, tipo A (IA i) ou tipo B (IB i).",

        "Rr x rr: metade dos filhos recebe r dos dois (rr, Rh negativo) "
        "e metade recebe R (Rr, Rh positivo). Chance de 50%.",

        "Aa x Aa gera AA, Aa, Aa e aa. Somente aa e albino: "
        "1 em 4, ou seja, 25%.",

        "A mae sempre doa um X. O pai doa X ou Y. Se o espermatozoide "
        "tiver Y, nasce menino. Se tiver X, nasce menina."
    };

    // RESPOSTAS CORRETAS DAS CASAS
    int respostas_casas[NUM_CASAS] = { 2, 3, 1, 4, 1 };

    // FILA DE EVENTOS
    ALLEGRO_EVENT_QUEUE* event_queue =
        al_create_event_queue();

    if (!event_queue) {
        printf("Erro ao criar fila de eventos.\n");
        return 1;
    }

    al_register_event_source(
        event_queue,
        al_get_display_event_source(display)
    );

    al_register_event_source(
        event_queue,
        al_get_timer_event_source(timer)
    );

    al_register_event_source(
        event_queue,
        al_get_keyboard_event_source()
    );

    al_register_event_source(
        event_queue,
        al_get_mouse_event_source()
    );

    al_start_timer(timer);

    // LOOP PRINCIPAL
    while (1) {

        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

        // =====================================
        // TELA INICIAL
        // =====================================
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

                if (event.keyboard.keycode == ALLEGRO_KEY_LEFT ||
                    event.keyboard.keycode == ALLEGRO_KEY_A) {
                    selecao = 0;
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_RIGHT ||
                    event.keyboard.keycode == ALLEGRO_KEY_D) {
                    selecao = 1;
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_ENTER &&
                    selecao != -1) {
                    escolhido = selecao;
                }
            }

            // CARREGA PERSONAGEM ESCOLHIDO
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

                    float cx =
                        (seta_x[selecao] - recorte_x) * esc_titulo;

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

        // =====================================
        // FASE 1
        // =====================================
        else if (estado == ESTADO_FASE_1) {

            // ABRIR E FECHAR POP-UP
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {

                if (event.keyboard.keycode == ALLEGRO_KEY_E) {

                    if (popup_aberto != -1) {

                        popup_aberto = -1;
                        desafio_iniciado = 0;
                    }
                    else if (local_perto != -1) {

                        popup_aberto = local_perto;

                        // CASA
                        if (popup_aberto == 2) {
                            pagina_mendel = 0;
                        }

                        // LABORATORIO
                        if (popup_aberto == 0) {

                            int total_coletadas = 0;

                            for (int i = 0; i < NUM_ERVILHAS; i++) {

                                if (ervilhas[i].coletada) {
                                    total_coletadas++;
                                }
                            }

                            // LIBERA O TESTE SOMENTE COM TODAS AS ERVILHAS
                            if (total_coletadas == NUM_ERVILHAS) {

                                pergunta_mendel = 0;
                                desafio_iniciado = 1;
                                desafio_concluido = 0;

                                inicio_desafio = al_get_time();
                                tempo_restante = 45;

                                definir_mensagem(
                                    mensagem_desafio,
                                    sizeof(mensagem_desafio),
                                    "Responda as 3 perguntas antes que o tempo acabe!"
                                );
                            }
                            else {

                                desafio_iniciado = 0;
                                desafio_concluido = 0;

                                snprintf(
                                    mensagem_desafio,
                                    sizeof(mensagem_desafio),
                                    "Voce precisa colher todas as ervilhas! (%d/%d)",
                                    total_coletadas,
                                    NUM_ERVILHAS
                                );
                            }
                        }
                    }
                }

                // PASSAR PAGINA DO LIVRO COM P
                if (event.keyboard.keycode == ALLEGRO_KEY_P &&
                    popup_aberto == 2) {

                    pagina_mendel++;

                    if (pagina_mendel >= 3) {
                        pagina_mendel = 0;
                    }
                }

                // RESPOSTAS DO TESTE: TECLAS 1, 2, 3 OU 4
                if (popup_aberto == 0 &&
                    desafio_iniciado &&
                    !desafio_concluido) {

                    int resposta = 0;

                    if (event.keyboard.keycode == ALLEGRO_KEY_1 ||
                        event.keyboard.keycode == ALLEGRO_KEY_PAD_1) {
                        resposta = 1;
                    }
                    else if (event.keyboard.keycode == ALLEGRO_KEY_2 ||
                        event.keyboard.keycode == ALLEGRO_KEY_PAD_2) {
                        resposta = 2;
                    }
                    else if (event.keyboard.keycode == ALLEGRO_KEY_3 ||
                        event.keyboard.keycode == ALLEGRO_KEY_PAD_3) {
                        resposta = 3;
                    }
                    else if (event.keyboard.keycode == ALLEGRO_KEY_4 ||
                        event.keyboard.keycode == ALLEGRO_KEY_PAD_4) {
                        resposta = 4;
                    }

                    if (resposta != 0) {

                        if (resposta ==
                            respostas_corretas[pergunta_mendel]) {

                            pergunta_mendel++;

                            if (pergunta_mendel >= 3) {

                                desafio_concluido = 1;
                                fase2_liberada = 1;
                                desafio_iniciado = 0;

                                definir_mensagem(
                                    mensagem_desafio,
                                    sizeof(mensagem_desafio),
                                    "Parabens! Voce concluiu o Teste de Mendel!"
                                );
                            }
                            else {

                                definir_mensagem(
                                    mensagem_desafio,
                                    sizeof(mensagem_desafio),
                                    "Resposta correta! Proxima pergunta."
                                );
                            }
                        }
                        else {

                            definir_mensagem(
                                mensagem_desafio,
                                sizeof(mensagem_desafio),
                                "Resposta incorreta! Tente novamente."
                            );
                        }
                    }
                }

                // FECHAR COM ESC
                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {

                    popup_aberto = -1;
                    desafio_iniciado = 0;
                }
            }

            // =====================================
            // CLIQUE NAS ERVILHAS
            // =====================================
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

            // =====================================
            // ATUALIZACAO DO JOGO
            // =====================================
            if (event.type == ALLEGRO_EVENT_TIMER) {

                andando = 0;

                // MOVIMENTACAO
                if (popup_aberto == -1) {
                    mover_personagem(&x, &y, &direcao, &andando,
                        velocidade, largura, altura);
                }

                // ANIMACAO
                animar(andando, &contador, &passo);

                // CHEGOU PERTO DA SETA: VAI PARA A FASE 2
                if (fase2_liberada &&
                    popup_aberto == -1 &&
                    x + largura > SETA_FASE2_X - DISTANCIA_PERTO &&
                    x < SETA_FASE2_X + SETA_FASE2_TAM + DISTANCIA_PERTO &&
                    y + altura > SETA_FASE2_Y - DISTANCIA_PERTO &&
                    y < SETA_FASE2_Y + SETA_FASE2_TAM + DISTANCIA_PERTO) {

                    estado = ESTADO_FASE_2;

                    x = 860;
                    y = 380;
                    direcao = COL_ESQUERDA;
                    popup_casa = -1;
                    casa_fechada = -1;

                    continue;
                }

                // VERIFICA SE ESTA PERTO DE ALGUM LOCAL
                local_perto = -1;

                for (int i = 0; i < NUM_LOCAIS; i++) {

                    if (x + largura >
                        locais[i].x1 - DISTANCIA_PERTO &&

                        x < locais[i].x2 + DISTANCIA_PERTO &&

                        y + altura >
                        locais[i].y1 - DISTANCIA_PERTO &&

                        y < locais[i].y2 + DISTANCIA_PERTO) {

                        local_perto = i;
                    }
                }

                // =====================================
                // TEMPORIZADOR DO DESAFIO
                // =====================================
                if (popup_aberto == 0 &&
                    desafio_iniciado &&
                    !desafio_concluido) {

                    double decorrido =
                        al_get_time() - inicio_desafio;

                    tempo_restante =
                        TEMPO_DESAFIO - (int)decorrido;

                    if (tempo_restante < 0) {
                        tempo_restante = 0;
                    }

                    // TEMPO ESGOTADO: VOLTA PARA A PRIMEIRA PERGUNTA
                    if (decorrido >= TEMPO_DESAFIO) {

                        pergunta_mendel = 0;

                        inicio_desafio = al_get_time();

                        tempo_restante = 45;

                        definir_mensagem(
                            mensagem_desafio,
                            sizeof(mensagem_desafio),
                            "Tempo esgotado! O teste voltou ao inicio. Tente novamente!"
                        );
                    }
                }

                // =====================================
                // DESENHO DO MAPA
                // =====================================
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
                // SETA PARA A PROXIMA FASE (FICA PARADA)
                if (fase2_liberada) {

                    al_draw_scaled_rotated_bitmap(
                        seta,
                        al_get_bitmap_width(seta) / 2.0f,
                        al_get_bitmap_height(seta) / 2.0f,
                        SETA_FASE2_X + SETA_FASE2_TAM / 2.0f,
                        SETA_FASE2_Y + SETA_FASE2_TAM / 2.0f,
                        SETA_FASE2_TAM / (float)al_get_bitmap_width(seta),
                        SETA_FASE2_TAM / (float)al_get_bitmap_height(seta),
                        1.5707963f,
                        0
                    );
                }

                // DICA PARA ENTRAR NOS LOCAIS
                if (local_perto != -1 &&
                    popup_aberto == -1) {

                    al_draw_filled_rectangle(
                        262, 740, 762, 780,
                        al_map_rgba(0, 0, 0, 180)
                    );

                    al_draw_text(
                        font,
                        al_map_rgb(255, 255, 255),
                        LARGURA_TELA / 2,
                        756,
                        ALLEGRO_ALIGN_CENTRE,
                        "Aperte E para entrar"
                    );
                }

                // =====================================
                // POP-UP
                // =====================================
                if (popup_aberto != -1) {

                    al_draw_filled_rectangle(
                        0, 0,
                        LARGURA_TELA,
                        ALTURA_TELA,
                        al_map_rgba(0, 0, 0, 150)
                    );

                    // =================================
                    // FAZENDA DE ERVILHAS
                    // =================================
                    if (popup_aberto == 1) {

                        al_draw_bitmap(
                            fundo_fazenda,
                            FAZ_X,
                            FAZ_Y,
                            0
                        );

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

                    // =================================
                    // CASA - LIVRO DE MENDEL
                    // =================================
                    else if (popup_aberto == 2) {

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

                        al_draw_multiline_text(
                            font,
                            al_map_rgb(40, 40, 40),
                            130,
                            450,
                            760,
                            15,
                            ALLEGRO_ALIGN_LEFT,
                            textos_mendel[pagina_mendel]
                        );
                    }

                    // =================================
                    // LABORATORIO - TESTE DE MENDEL
                    // =================================
                    else if (popup_aberto == 0) {

                        // PAINEL PRINCIPAL
                        al_draw_filled_rounded_rectangle(
                            112, 105, 912, 695,
                            18, 18,
                            al_map_rgb(230, 239, 226)
                        );

                        al_draw_rounded_rectangle(
                            112, 105, 912, 695,
                            18, 18,
                            al_map_rgb(53, 92, 62),
                            5
                        );

                        al_draw_text(
                            font,
                            al_map_rgb(35, 65, 43),
                            LARGURA_TELA / 2,
                            130,
                            ALLEGRO_ALIGN_CENTRE,
                            "LABORATORIO DE GENETICA"
                        );

                        al_draw_text(
                            font,
                            al_map_rgb(35, 65, 43),
                            LARGURA_TELA / 2,
                            155,
                            ALLEGRO_ALIGN_CENTRE,
                            "O TESTE DE MENDEL"
                        );

                        // CONTAGEM DAS ERVILHAS
                        int total_coletadas = 0;

                        for (int i = 0; i < NUM_ERVILHAS; i++) {

                            if (ervilhas[i].coletada) {
                                total_coletadas++;
                            }
                        }

                        // BLOQUEIO DO LABORATORIO
                        if (total_coletadas < NUM_ERVILHAS) {

                            al_draw_text(
                                font,
                                al_map_rgb(145, 55, 45),
                                LARGURA_TELA / 2,
                                285,
                                ALLEGRO_ALIGN_CENTRE,
                                "LABORATORIO BLOQUEADO!"
                            );

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                330,
                                ALLEGRO_ALIGN_CENTRE,
                                "Voce precisa colher as 8 ervilhas na Fazenda."
                            );

                            al_draw_textf(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                365,
                                ALLEGRO_ALIGN_CENTRE,
                                "Ervilhas coletadas: %d/%d",
                                total_coletadas,
                                NUM_ERVILHAS
                            );

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                430,
                                ALLEGRO_ALIGN_CENTRE,
                                "Volte a Fazenda e colete todas para liberar o teste!"
                            );
                        }

                        // DESAFIO CONCLUIDO
                        else if (desafio_concluido) {

                            al_draw_text(
                                font,
                                al_map_rgb(40, 120, 55),
                                LARGURA_TELA / 2,
                                290,
                                ALLEGRO_ALIGN_CENTRE,
                                "PARABENS! VOCE CONSEGUIU!"
                            );

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                340,
                                ALLEGRO_ALIGN_CENTRE,
                                "Voce concluiu as 3 perguntas dentro do tempo."
                            );

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                375,
                                ALLEGRO_ALIGN_CENTRE,
                                "Voce passou de fase e dominou os conceitos de Mendel!"
                            );
                        }

                        // DESAFIO EM ANDAMENTO
                        else if (desafio_iniciado) {

                            // TEMPORIZADOR
                            al_draw_filled_rectangle(
                                750, 185,
                                865, 225,
                                al_map_rgb(45, 75, 48)
                            );

                            al_draw_textf(
                                font,
                                al_map_rgb(255, 255, 255),
                                807, 198,
                                ALLEGRO_ALIGN_CENTRE,
                                "TEMPO: %02d",
                                tempo_restante
                            );

                            // PROGRESSO
                            al_draw_textf(
                                font,
                                al_map_rgb(35, 65, 43),
                                160, 195,
                                ALLEGRO_ALIGN_LEFT,
                                "PERGUNTA %d DE 3",
                                pergunta_mendel + 1
                            );

                            // ENUNCIADO
                            al_draw_multiline_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                170, 270,
                                680,
                                25,
                                ALLEGRO_ALIGN_LEFT,
                                perguntas_mendel[pergunta_mendel]
                            );

                            // ALTERNATIVAS
                            for (int i = 0; i < 4; i++) {

                                float y_alt = 370 + i * 48;

                                al_draw_filled_rounded_rectangle(
                                    190, y_alt,
                                    834, y_alt + 36,
                                    7, 7,
                                    al_map_rgb(255, 255, 255)
                                );

                                al_draw_rounded_rectangle(
                                    190, y_alt,
                                    834, y_alt + 36,
                                    7, 7,
                                    al_map_rgb(130, 160, 125),
                                    2
                                );

                                al_draw_text(
                                    font,
                                    al_map_rgb(35, 65, 43),
                                    215,
                                    y_alt + 11,
                                    ALLEGRO_ALIGN_LEFT,
                                    alternativas_mendel[pergunta_mendel][i]
                                );
                            }

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                590,
                                ALLEGRO_ALIGN_CENTRE,
                                "Digite 1, 2, 3 ou 4 para responder."
                            );

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                LARGURA_TELA / 2,
                                620,
                                ALLEGRO_ALIGN_CENTRE,
                                mensagem_desafio
                            );
                        }

                        // INSTRUCAO PARA FECHAR
                        al_draw_text(
                            font,
                            al_map_rgb(35, 65, 43),
                            LARGURA_TELA / 2,
                            660,
                            ALLEGRO_ALIGN_CENTRE,
                            "Aperte E ou ESC para fechar"
                        );
                    }

                    // OUTROS LOCAIS
                    else {

                        al_draw_filled_rectangle(
                            212, 250,
                            812, 550,
                            al_map_rgb(255, 255, 255)
                        );

                        al_draw_rectangle(
                            212, 250,
                            812, 550,
                            al_map_rgb(3, 169, 244),
                            4
                        );
                    }
                }

                al_flip_display();
            }
        }
        // =====================================
        // FASE 2
        // =====================================
        else if (estado == ESTADO_FASE_2) {

            // TECLADO
            if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
                popup_casa != -1) {

                int tecla = event.keyboard.keycode;

                // FECHAR COM E OU ESC
                if (tecla == ALLEGRO_KEY_E ||
                    tecla == ALLEGRO_KEY_ESCAPE) {

                    casa_fechada = popup_casa;
                    popup_casa = -1;
                }
                // RESPONDER COM 1, 2, 3 OU 4
                else if (!casa_ok[popup_casa]) {

                    int resposta = tecla_resposta(tecla);

                    if (resposta != 0) {

                        if (resposta == respostas_casas[popup_casa]) {

                            casa_ok[popup_casa] = 1;

                            definir_mensagem(
                                mensagem_casa,
                                sizeof(mensagem_casa),
                                "Resposta correta!"
                            );
                        }
                        else {

                            definir_mensagem(
                                mensagem_casa,
                                sizeof(mensagem_casa),
                                "Resposta incorreta! Tente novamente."
                            );
                        }
                    }
                }
            }

            // ATUALIZACAO E DESENHO
            if (event.type == ALLEGRO_EVENT_TIMER) {

                andando = 0;

                if (popup_casa == -1) {
                    mover_personagem(&x, &y, &direcao, &andando,
                        velocidade, largura, altura);
                }

                animar(andando, &contador, &passo);

                // VERIFICA SE ESTA PERTO DE ALGUMA CASA
                int casa_perto = -1;

                for (int i = 0; i < NUM_CASAS; i++) {

                    if (x + largura > casas[i].x1 - DISTANCIA_PERTO &&
                        x < casas[i].x2 + DISTANCIA_PERTO &&
                        y + altura > casas[i].y1 - DISTANCIA_PERTO &&
                        y < casas[i].y2 + DISTANCIA_PERTO) {

                        casa_perto = i;
                    }
                }

                // SAIU DE PERTO: A CASA PODE ABRIR DE NOVO
                if (casa_perto == -1) {
                    casa_fechada = -1;
                }

                // ABRE O MINI JOGO AO CHEGAR PERTO
                if (popup_casa == -1 &&
                    casa_perto != -1 &&
                    casa_perto != casa_fechada &&
                    !casa_ok[casa_perto]) {

                    popup_casa = casa_perto;

                    definir_mensagem(
                        mensagem_casa,
                        sizeof(mensagem_casa),
                        "Digite 1, 2, 3 ou 4 para responder."
                    );
                }

                int total_casas = 0;

                for (int i = 0; i < NUM_CASAS; i++) {
                    if (casa_ok[i]) {
                        total_casas++;
                    }
                }

                // MAPA E PERSONAGEM
                al_draw_bitmap(fundo_fase2, 0, 0, 0);

                desenhar_personagem(sheet, direcao, andando,
                    ciclo[passo], x, y, largura, altura);

                // PLACAR
                al_draw_filled_rectangle(
                    10, 10, 250, 40,
                    al_map_rgba(0, 0, 0, 170)
                );

                al_draw_textf(
                    font,
                    al_map_rgb(255, 255, 255),
                    20, 21,
                    ALLEGRO_ALIGN_LEFT,
                    "Casas concluidas: %d/%d",
                    total_casas,
                    NUM_CASAS
                );

                if (total_casas == NUM_CASAS && popup_casa == -1) {

                    al_draw_filled_rectangle(
                        162, 740, 862, 780,
                        al_map_rgba(0, 0, 0, 180)
                    );

                    al_draw_text(
                        font,
                        al_map_rgb(168, 230, 29),
                        LARGURA_TELA / 2,
                        756,
                        ALLEGRO_ALIGN_CENTRE,
                        "PARABENS! Voce concluiu a Fase 2!"
                    );
                }

                // POP-UP DO MINI JOGO
                if (popup_casa != -1) {

                    al_draw_filled_rectangle(
                        0, 0,
                        LARGURA_TELA,
                        ALTURA_TELA,
                        al_map_rgba(0, 0, 0, 150)
                    );

                    al_draw_filled_rounded_rectangle(
                        112, 105, 912, 695,
                        18, 18,
                        al_map_rgb(230, 239, 226)
                    );

                    al_draw_rounded_rectangle(
                        112, 105, 912, 695,
                        18, 18,
                        al_map_rgb(53, 92, 62),
                        5
                    );

                    al_draw_textf(
                        font,
                        al_map_rgb(35, 65, 43),
                        LARGURA_TELA / 2,
                        130,
                        ALLEGRO_ALIGN_CENTRE,
                        "%s",
                        casas[popup_casa].nome
                    );

                    al_draw_textf(
                        font,
                        al_map_rgb(35, 65, 43),
                        LARGURA_TELA / 2,
                        155,
                        ALLEGRO_ALIGN_CENTRE,
                        "MINI JOGO: %s",
                        casas[popup_casa].descricao
                    );

                    al_draw_multiline_text(
                        font,
                        al_map_rgb(35, 65, 43),
                        170, 230,
                        680,
                        25,
                        ALLEGRO_ALIGN_LEFT,
                        perguntas_casas[popup_casa]
                    );

                    if (!casa_ok[popup_casa]) {

                        // ALTERNATIVAS
                        for (int i = 0; i < 4; i++) {

                            float y_alt = 370 + i * 48;

                            al_draw_filled_rounded_rectangle(
                                190, y_alt,
                                834, y_alt + 36,
                                7, 7,
                                al_map_rgb(255, 255, 255)
                            );

                            al_draw_rounded_rectangle(
                                190, y_alt,
                                834, y_alt + 36,
                                7, 7,
                                al_map_rgb(130, 160, 125),
                                2
                            );

                            al_draw_text(
                                font,
                                al_map_rgb(35, 65, 43),
                                215,
                                y_alt + 11,
                                ALLEGRO_ALIGN_LEFT,
                                alternativas_casas[popup_casa][i]
                            );
                        }
                    }
                    else {

                        // EXPLICACAO DEPOIS DE ACERTAR
                        al_draw_text(
                            font,
                            al_map_rgb(40, 120, 55),
                            LARGURA_TELA / 2,
                            370,
                            ALLEGRO_ALIGN_CENTRE,
                            "RESPOSTA CORRETA!"
                        );

                        al_draw_multiline_text(
                            font,
                            al_map_rgb(35, 65, 43),
                            170, 410,
                            680,
                            25,
                            ALLEGRO_ALIGN_LEFT,
                            explicacoes_casas[popup_casa]
                        );
                    }

                    al_draw_text(
                        font,
                        al_map_rgb(35, 65, 43),
                        LARGURA_TELA / 2,
                        600,
                        ALLEGRO_ALIGN_CENTRE,
                        mensagem_casa
                    );

                    al_draw_text(
                        font,
                        al_map_rgb(35, 65, 43),
                        LARGURA_TELA / 2,
                        660,
                        ALLEGRO_ALIGN_CENTRE,
                        "Aperte E ou ESC para fechar"
                    );
                }

                al_flip_display();
            }
        }
    }

    // =====================================
    // LIBERAR MEMORIA
    // =====================================
    al_destroy_bitmap(titulo);
    al_destroy_bitmap(seta);
    al_destroy_bitmap(sheet_pedro);
    al_destroy_bitmap(sheet_amanda);
    al_destroy_bitmap(fundo);
    al_destroy_bitmap(fundo_fazenda);
    al_destroy_bitmap(livro_mendel);
    al_destroy_bitmap(fundo_fase2);

    for (int i = 0; i < NUM_ERVILHAS; i++) {
        al_destroy_bitmap(ervilhas[i].imagem);
    }

    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}
