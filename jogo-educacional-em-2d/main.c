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

// Cada lugar do mapa: retângulo (x1,y1 até x2,y2), nome e texto do pop-up
typedef struct {
    float x1, y1, x2, y2;
    const char* nome;
    const char* descricao;
} Local;

int main() {

    // ===== 1. LIGAR AS PARTES DO ALLEGRO =====
    al_init();
    al_init_font_addon();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    al_set_window_title(display, "PixelGENETICS");

    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

    // ===== 2. CARREGAR IMAGENS =====
    ALLEGRO_BITMAP* personagem = al_load_bitmap("personagem.png");
    if (personagem == NULL) {
        printf("Erro: nao encontrei o arquivo personagem.png\n");
        return 1;
    }

    ALLEGRO_BITMAP* fundo = al_load_bitmap("tela_fundo.png");
    if (fundo == NULL) {
        printf("Erro: nao encontrei o arquivo tela_fundo.png\n");
        return 1;
    }

    // ===== 3. OS LUGARES DO MAPA (posições medidas na imagem de fundo) =====
    // Obs.: a fonte embutida do Allegro não tem acentos, por isso "LABORATORIO"
    Local locais[NUM_LOCAIS] = {
        { 148, 128, 436, 299, "LABORATORIO",""},
        { 734,  76, 955, 238, "FAZENDA DE ERVINHAS",""},
        { 734, 528, 942, 676, "TESTE DE MESA",""}
    };

    // ===== 4. VARIÁVEIS DO PERSONAGEM =====
    int escala = 4;
    float largura = al_get_bitmap_width(personagem) * escala;    // 80
    float altura = al_get_bitmap_height(personagem) * escala;    // 80
    float x = 480;
    float y = 380;
    float velocidade = 4;

    int local_perto = -1;   // qual lugar está perto (-1 = nenhum)
    int popup_aberto = -1;  // qual pop-up está aberto (-1 = nenhum)

    // ===== 5. FILA DE EVENTOS =====
    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());   // pra tecla E
    al_start_timer(timer);

    // ===== 6. LAÇO DO JOGO =====
    while (true) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

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
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                popup_aberto = -1;                  // ESC também fecha
            }
        }

        // --- Cada quadro (60 vezes por segundo) ---
        if (event.type == ALLEGRO_EVENT_TIMER) {

            // Movimento (só quando o pop-up está fechado)
            if (popup_aberto == -1) {
                ALLEGRO_KEYBOARD_STATE teclado;
                al_get_keyboard_state(&teclado);

                if (al_key_down(&teclado, ALLEGRO_KEY_W)) {
                    y = y - velocidade;
                }
                if (al_key_down(&teclado, ALLEGRO_KEY_S)) {
                    y = y + velocidade;
                }
                if (al_key_down(&teclado, ALLEGRO_KEY_A)) {
                    x = x - velocidade;
                }
                if (al_key_down(&teclado, ALLEGRO_KEY_D)) {
                    x = x + velocidade;
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

            // Personagem
            al_draw_scaled_bitmap(personagem,
                0, 0, al_get_bitmap_width(personagem), al_get_bitmap_height(personagem),
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
            }

            al_flip_display();
        }
    }

    // ===== 7. LIMPEZA =====
    al_destroy_bitmap(personagem);
    al_destroy_bitmap(fundo);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);

    return 0;
}