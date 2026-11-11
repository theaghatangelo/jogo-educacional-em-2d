#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>   // retângulos (pop-up e caixa de dica)

// ===== CONFIGURAÇÕES DA DEMO =====
#define LARGURA_TELA 1024     // mesmo tamanho da imagem tela_fundo.png
#define ALTURA_TELA 800
#define DISTANCIA_PERTO 15    // quantos pixels de distância contam como "perto"
#define NUM_LOCAIS 3

// ===== CONFIGURAÇÕES DO SPRITE SHEET (800x600: 4 colunas x 3 linhas) =====
#define TAM_SPRITE 200        // cada quadro tem 200x200 pixels
#define ESCALA_SPRITE 0.5f    // 200 * 0.5 = 100 pixels na tela (mude aqui pra aumentar/diminuir)
#define TICKS_POR_PASSO 8     // a cada quantos quadros (60/s) troca a perna ao andar

// Colunas do sprite sheet = direção que o personagem está olhando.
// Se ao testar ele andar "de costas" pro lado errado, é só trocar os números.
#define COL_ESQUERDA 0
#define COL_DIREITA  1
#define COL_FRENTE   2
#define COL_COSTAS   3

// Linhas do sprite sheet = parado / perna levantada / outra perna levantada
#define LIN_PARADO 0

// ===== TELA INICIAL =====
#define TITULO_LARGURA 1920   // tamanho da imagem tela_inicial.png
#define TITULO_ALTURA 1080

// Estados do jogo: é assim que o programa sabe em qual "tela" está
typedef enum {
    ESTADO_TELA_INICIAL,
    ESTADO_FASE_1
} Estado;

// Cada lugar do mapa: retângulo (x1,y1 até x2,y2), nome e texto do pop-up
typedef struct {
    float x1, y1, x2, y2;
    const char* nome;
    const char* descricao;
} Local;

// Uma área retangular (usada para os botões Pedro / Amanda na imagem da tela inicial)
typedef struct {
    float x1, y1, x2, y2;
} Area;

// Diz se o ponto (px, py) está dentro da área
static int dentro(Area a, float px, float py) {
    return px >= a.x1 && px <= a.x2 && py >= a.y1 && py <= a.y2;
}

int main() {

    //Inicialização do allegro
    al_init();

    //Inicialização de addon
    al_init_font_addon();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();      // novo: a tela inicial usa o mouse

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    al_set_window_title(display, "PixelGENETICS");

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

    //Imagens
    ALLEGRO_BITMAP* titulo = al_load_bitmap("tela_inicial.png");
    if (titulo == NULL) {
        printf("Erro: nao encontrei o arquivo tela_inicial.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* sheet_pedro = al_load_bitmap("pedro_spritesheet.png");
    if (sheet_pedro == NULL) {
        printf("Erro: nao encontrei o arquivo pedro_spritesheet.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* sheet_amanda = al_load_bitmap("amanda_spritesheet.png");
    if (sheet_amanda == NULL) {
        printf("Erro: nao encontrei o arquivo amanda_spritesheet.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* fundo = al_load_bitmap("tela_fundo.png");
    if (fundo == NULL) {
        printf("Erro: nao encontrei o arquivo tela_fundo.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* seta = al_load_bitmap("seta.png");
    if (seta == NULL) {
        printf("Erro: nao encontrei o arquivo seta.png\n");
        return 1;
    }

    // ===== ESTADO DO JOGO =====
    Estado estado = ESTADO_TELA_INICIAL;   // o jogo começa na tela inicial

    // ===== TELA INICIAL: como encaixar a imagem 1920x1080 na janela 1024x800 =====
    // A imagem é mais larga que a janela. Em vez de esticar (deformaria o desenho),
    // ajustamos pela altura e cortamos um pouco das laterais (onde só tem céu).
    float esc_titulo = (float)ALTURA_TELA / TITULO_ALTURA;      // ~0.74
    float recorte_w = LARGURA_TELA / esc_titulo;                // parte da imagem que aparece
    float recorte_x = (TITULO_LARGURA - recorte_w) / 2;         // corta igual dos dois lados

    // Áreas clicáveis (em coordenadas da imagem original 1920x1080): personagem + botão com o nome
    Area opcoes[2] = {
        { 640, 300,  950, 745 },    // 0 = Pedro
        { 950, 300, 1260, 745 }     // 1 = Amanda
    };
    int selecao = -1;    // opção destacada (-1 = nenhuma)
    float seta_x[2] = { 806, 1094 };   // 0 = Pedro, 1 = Amanda
    float seta_y = 748;
    float seta_tam = 48;               // tamanho da setinha na tela, em pixels

    // ===== 3. OS LUGARES DO MAPA (posições medidas na imagem de fundo) =====
    // Obs.: a fonte embutida do Allegro não tem acentos, por isso "LABORATORIO"
    Local locais[NUM_LOCAIS] = {
        { 148, 128, 436, 299, "LABORATORIO","" },
        { 734,  76, 955, 238, "FAZENDA DE ERVINHAS","" },
        { 734, 528, 942, 676, "TESTE DE MESA","" }
    };

    // Variaveis
    ALLEGRO_BITMAP* sheet = NULL;                // sprite sheet do personagem escolhido
    float largura = TAM_SPRITE * ESCALA_SPRITE;  // 100
    float altura = TAM_SPRITE * ESCALA_SPRITE;   // 100
    float x = 480;
    float y = 380;
    float velocidade = 4;

    // Animação do personagem
    int direcao = COL_FRENTE;          // coluna do sprite sheet
    int andando = 0;                   // 1 = está se movendo neste quadro
    int contador = 0;                  // conta quadros pra saber a hora de trocar a perna
    int passo = 0;                     // posição no ciclo de caminhada
    int ciclo[4] = { 1, 0, 2, 0 };     // linhas usadas ao andar: perna, parado, outra perna, parado

    int local_perto = -1;   // qual lugar está perto (-1 = nenhum)
    int popup_aberto = -1;  // qual pop-up está aberto (-1 = nenhum)

    //Fase 1  
    int coletou_verde = 0;
    int coletou_amarela = 0;

    //Fila de Eventos
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());   // pra tecla E
    al_register_event_source(event_queue, al_get_mouse_event_source());      // pro clique na tela inicial
    al_start_timer(timer);

    // ===== 6. LAÇO DO JOGO =====
    while (true) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

        // =====================================================
        //  TELA INICIAL: escolher o personagem
        // =====================================================
        if (estado == ESTADO_TELA_INICIAL) {

            int escolhido = -1;   // vira 0 (Pedro) ou 1 (Amanda) quando a pessoa confirma

            // Mouse passando por cima: destaca a opção
            if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
                // converte a posição do mouse (janela) para posição na imagem original
                float mx = event.mouse.x / esc_titulo + recorte_x;
                float my = event.mouse.y / esc_titulo;
                selecao = -1;
                for (int i = 0; i < 2; i++) {
                    if (dentro(opcoes[i], mx, my)) {
                        selecao = i;
                    }
                }
            }

            // Clique do mouse: confirma a opção
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && event.mouse.button == 1) {
                float mx = event.mouse.x / esc_titulo + recorte_x;
                float my = event.mouse.y / esc_titulo;
                for (int i = 0; i < 2; i++) {
                    if (dentro(opcoes[i], mx, my)) {
                        escolhido = i;
                    }
                }
            }

            // Teclado (opcional): setas escolhem, ENTER confirma
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
                    selecao = 0;
                }
                if (event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
                    selecao = 1;
                }
                if (event.keyboard.keycode == ALLEGRO_KEY_ENTER && selecao != -1) {
                    escolhido = selecao;
                }
            }

            // Escolheu! Guarda o sprite sheet certo e vai para a Fase 1
            if (escolhido != -1) {
                if (escolhido == 0) {
                    sheet = sheet_pedro;
                }
                else {
                    sheet = sheet_amanda;
                }
                estado = ESTADO_FASE_1;
            }

            // Desenho da tela inicial (60 vezes por segundo)
            if (event.type == ALLEGRO_EVENT_TIMER && estado == ESTADO_TELA_INICIAL) {
                al_draw_scaled_bitmap(titulo,
                    recorte_x, 0, recorte_w, TITULO_ALTURA,
                    0, 0, LARGURA_TELA, ALTURA_TELA, 0);

                if (selecao != -1) {
                    float cx = (seta_x[selecao] - recorte_x) * esc_titulo;   // centro da seta na tela
                    float topo = seta_y * esc_titulo;
                    al_draw_scaled_bitmap(seta,
                        0, 0, al_get_bitmap_width(seta), al_get_bitmap_height(seta),
                        cx - seta_tam / 2, topo, seta_tam, seta_tam, 0);
                }

                al_flip_display();
            }
        }

        // =====================================================
        //  FASE 1
        // =====================================================
        else if (estado == ESTADO_FASE_1) {

            // --- Tecla apertada (acontece UMA vez por aperto) ---
            if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (event.keyboard.keycode == ALLEGRO_KEY_E) {
                    if (popup_aberto != -1) {
                        popup_aberto = -1;              // pop-up aberto: E fecha
                    }
                    else if (local_perto != -1) {
                        popup_aberto = local_perto;     // perto de um lugar: E abre
                    }
                }
                if (popup_aberto == 1 && event.keyboard.keycode == ALLEGRO_KEY_1) {
                    coletou_verde = 1;
                }
                if (popup_aberto == 1 && event.keyboard.keycode == ALLEGRO_KEY_2) {
                    coletou_amarela = 1;
                }

                if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    popup_aberto = -1;                  // ESC também fecha
                }
            }

            // --- Cada quadro (60 vezes por segundo) ---
            if (event.type == ALLEGRO_EVENT_TIMER) {

                andando = 0;   // só vira 1 se alguma tecla de movimento estiver apertada

                // Movimento (só quando o pop-up está fechado)
                if (popup_aberto == -1) {
                    ALLEGRO_KEYBOARD_STATE teclado;
                    al_get_keyboard_state(&teclado);

                    if (al_key_down(&teclado, ALLEGRO_KEY_W)) {
                        y = y - velocidade;
                        direcao = COL_COSTAS;
                        andando = 1;
                    }
                    if (al_key_down(&teclado, ALLEGRO_KEY_S)) {
                        y = y + velocidade;
                        direcao = COL_FRENTE;
                        andando = 1;
                    }
                    if (al_key_down(&teclado, ALLEGRO_KEY_A)) {
                        x = x - velocidade;
                        direcao = COL_ESQUERDA;
                        andando = 1;
                    }
                    if (al_key_down(&teclado, ALLEGRO_KEY_D)) {
                        x = x + velocidade;
                        direcao = COL_DIREITA;
                        andando = 1;
                    }

                    // Não deixa sair da tela
                    if (x < 0) {
                        x = 0;
                    }
                    if (y < 0) {
                        y = 0;
                    }
                    if (x > LARGURA_TELA - largura) {
                        x = LARGURA_TELA - largura;
                    }
                    if (y > ALTURA_TELA - altura) {
                        y = ALTURA_TELA - altura;
                    }
                }

                // Animação: enquanto anda, alterna as pernas; parado, volta pro quadro de descanso
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

                // Descobre se o personagem está perto de algum lugar
                local_perto = -1;
                for (int i = 0; i < NUM_LOCAIS; i++) {
                    if (x + largura > locais[i].x1 - DISTANCIA_PERTO &&
                        x < locais[i].x2 + DISTANCIA_PERTO &&
                        y + altura > locais[i].y1 - DISTANCIA_PERTO &&
                        y < locais[i].y2 + DISTANCIA_PERTO) {
                        local_perto = i;
                    }
                }


                // ----- DESENHO -----
                al_draw_bitmap(fundo, 0, 0, 0);

                // Nome de cada lugar no meio do bloco azul
                for (int i = 0; i < NUM_LOCAIS; i++) {
                    float centro_x = (locais[i].x1 + locais[i].x2) / 2;
                    float centro_y = (locais[i].y1 + locais[i].y2) / 2;
                    al_draw_text(font, al_map_rgb(255, 255, 255), centro_x, centro_y - 4,
                        ALLEGRO_ALIGN_CENTRE, locais[i].nome);
                }

                // Personagem: recorta do sprite sheet só o quadro certo (coluna = direção, linha = perna)
                int linha = andando ? ciclo[passo] : LIN_PARADO;
                al_draw_scaled_bitmap(sheet,
                    direcao * TAM_SPRITE, linha * TAM_SPRITE, TAM_SPRITE, TAM_SPRITE,
                    x, y, largura, altura, 0);


                // Dica "Aperte E" quando está perto e o pop-up está fechado
                if (local_perto != -1 && popup_aberto == -1) {
                    al_draw_filled_rectangle(262, 740, 762, 780, al_map_rgba(0, 0, 0, 180));
                    al_draw_textf(font, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 756,
                        ALLEGRO_ALIGN_CENTRE, "Aperte E para entrar: %s", locais[local_perto].nome);
                }

                // Pop-up
                if (popup_aberto != -1) {
                    // Escurece o jogo atrás
                    al_draw_filled_rectangle(0, 0, LARGURA_TELA, ALTURA_TELA, al_map_rgba(0, 0, 0, 150));

                    // Caixa branca com borda azul
                    al_draw_filled_rectangle(212, 250, 812, 550, al_map_rgb(255, 255, 255));
                    al_draw_rectangle(212, 250, 812, 550, al_map_rgb(3, 169, 244), 4);

                    // Textos
                    al_draw_text(font, al_map_rgb(0, 0, 0), LARGURA_TELA / 2, 290,
                        ALLEGRO_ALIGN_CENTRE, locais[popup_aberto].nome);
                    al_draw_text(font, al_map_rgb(60, 60, 60), LARGURA_TELA / 2, 380,
                        ALLEGRO_ALIGN_CENTRE, locais[popup_aberto].descricao);
                    al_draw_text(font, al_map_rgb(120, 120, 120), LARGURA_TELA / 2, 510,
                        ALLEGRO_ALIGN_CENTRE, "");

                    //Ervilhas
                    if (popup_aberto == 1) {
                        al_draw_text(font, al_map_rgb(0, 0, 0), 512, 330, ALLEGRO_ALIGN_CENTRE, "Colete as ervilhas!");
                        if (coletou_verde == 0) {
                            al_draw_filled_circle(400, 420, 25, al_map_rgb(40, 180, 50));
                            al_draw_text(font, al_map_rgb(0, 0, 0), 400, 460, ALLEGRO_ALIGN_CENTRE, "Verde - Tecla 1");
                        }
                        else {
                            al_draw_text(font, al_map_rgb(0, 150, 0), 400, 420, ALLEGRO_ALIGN_CENTRE, "VERDE COLETADA");
                        }
                        if (coletou_amarela == 0) {
                            al_draw_filled_circle(620, 420, 25, al_map_rgb(255, 220, 0));
                            al_draw_text(font, al_map_rgb(0, 0, 0), 620, 460, ALLEGRO_ALIGN_CENTRE, "Amarela - Tecla 2");
                        }
                        else {
                            al_draw_text(font, al_map_rgb(180, 140, 0), 620, 420, ALLEGRO_ALIGN_CENTRE, "AMARELA COLETADA");
                        }
                    }

                }

                al_flip_display();
            }
        }
    }

    // ===== 7. LIMPEZA =====
    al_destroy_bitmap(titulo);
    al_destroy_bitmap(sheet_pedro);
    al_destroy_bitmap(sheet_amanda);
    al_destroy_bitmap(fundo);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}