#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <locale.h> // Biblioteca necessária para habilitar o suporte a acentuação (setlocale)

// Definições de constantes do jogo
#define NUM_ENEMIES 15
#define TIRO_INATIVO 0
#define TIRO_ATIVO 1
#define RAIO_TIRO 100
#define TEMPO_TIRO 2.0
#define BORDA_TIRO 4
#define SCORE_PENALTY 0.3
#define COMBUSTIVEL_MAX 100.0
#define TAXA_CONSUMO 8.0 // Quanto consome por segundo
#define VIDAS_MAX 3
#define TEMPO_INVULNERAVEL 2.0 // Segundos de invencibilidade após ser atingido

const float FPS = 100;  

const int SCREEN_W = 960;
const int SCREEN_H = 540;
const int HERO_W = 70;
const int HERO_H = 90;

// Variáveis globais para os recursos visuais do jogo
ALLEGRO_BITMAP *background = NULL;
ALLEGRO_BITMAP *nave = NULL;
ALLEGRO_BITMAP *meteoro = NULL;
ALLEGRO_BITMAP *img_combustivel = NULL; 
ALLEGRO_BITMAP *img_vidas = NULL; // Ponteiro para a imagem vidas.png
float background_y = 0;
ALLEGRO_COLOR BKG_COLOR;

// Definição dos ponteiros para os tamanhos de fonte da interface
ALLEGRO_FONT *FONT_52; // Fonte grande para o "GAME OVER"
ALLEGRO_FONT *FONT_32; // Fonte média para pontuação e botão de espaço
ALLEGRO_FONT *FONT_24; // Fonte menor para a instrução de saída (ESC)
ALLEGRO_FONT *FONT_18; // Fonte pequena para o "GÁS"

// Estrutura que controla o comportamento do escudo/tiro circular
typedef struct Tiro {
    int x, y;
    float raio;
    int modo;
    float timer;
    ALLEGRO_COLOR cor;
} Tiro;

// Estrutura base para naves
typedef struct Ship {
    int x, y;
    int vel;    
    ALLEGRO_COLOR cor;
    Tiro tiro;
} Ship;

// Estrutura de dados do jogador principal
typedef struct Hero {
    Ship ship;
    int dir_x;
    int dir_y;
    float score;
    float combustivel;
    int vidas;              
    float invulneravel_timer; 
} Hero;

// Estrutura de dados para os obstáculos (meteoros)
typedef struct Enemy {
    Ship ship;
    float raio;
    int active;
    int vel;
} Enemy; 

// Vetor global armazenando todos os meteoros da partida
Enemy enemies[NUM_ENEMIES];

// Estrutura para o combustível dinâmico
typedef struct Gasolina {
    float x, y;
    float vel;
    float raio; 
    int ativo;
} Gasolina;

// Criamos a variável global para o item
Gasolina item_gas;

// Reinicializa o estado do tiro/escudo do herói de volta ao repouso
void initTiro(Ship *s) {
    s->tiro.x = s->x;
    s->tiro.y = s->y;
    s->tiro.raio = 3;
    s->tiro.cor = s->cor;
    s->tiro.timer = TEMPO_TIRO;
    s->tiro.modo = TIRO_INATIVO;
}

// Inicializa configurações de cores padrão e carrega as fontes
void initGlobals() {
    BKG_COLOR = al_map_rgb(10, 10, 10);
    
    FONT_52 = al_load_font("Nowster Cute.otf", 52, 0);   
    FONT_32 = al_load_font("Hey Comic.otf", 32, 0);   
    FONT_24 = al_load_font("arial.ttf", 24, 0); 
    FONT_18 = al_load_font("arial.ttf", 18, 0); 
}

// Configura os valores iniciais do jogador principal
void initHero(Hero *s) {
    s->score = 0;
    s->combustivel = COMBUSTIVEL_MAX;
    s->vidas = VIDAS_MAX;             
    s->invulneravel_timer = 0;        
    s->ship.cor = al_map_rgb(100 + rand()%156, 100 + rand()%156, 100 + rand()%156);
    s->ship.x = SCREEN_W/2;
    s->ship.y = SCREEN_H - HERO_H - 10;
    s->ship.vel = 3;
    s->dir_x = 0;
    s->dir_y = 0;
    initTiro(&s->ship);
}

// Sorteia a física de um meteoro individual
void initSingleEnemy(int i) {
    enemies[i].active = 1;
    enemies[i].raio = 60 + rand() % 26; 
    enemies[i].vel = 1 + rand() % 3;
    
    enemies[i].ship.x = enemies[i].raio + rand() % (int)(SCREEN_W - enemies[i].raio * 2 - 60);
    enemies[i].ship.y = -(rand() % 200) - enemies[i].raio;
    
    enemies[i].ship.cor = al_map_rgb(150 + rand()%106, rand()%100, rand()%100);
}

void initEnemies() {
    for(int i = 0; i < NUM_ENEMIES; i++) {
        initSingleEnemy(i);
    }
}

void updateEnemies() {
    for(int i = 0; i < NUM_ENEMIES; i++) {
        if(enemies[i].active) {
            enemies[i].ship.y += enemies[i].vel; 
            if(enemies[i].active && enemies[i].ship.y - enemies[i].raio > SCREEN_H) {
                initSingleEnemy(i);
            }
        }
    }
}

void drawEnemies() {
    for(int i = 0; i < NUM_ENEMIES; i++) {
        if(enemies[i].active) {
            if(meteoro) {
                float diametro = enemies[i].raio * 2;
                al_draw_scaled_bitmap(meteoro,
                    0, 0, al_get_bitmap_width(meteoro), al_get_bitmap_height(meteoro),
                    enemies[i].ship.x - enemies[i].raio, enemies[i].ship.y - enemies[i].raio,       
                    diametro, diametro,                                                             
                    0);
            } else {
                al_draw_filled_circle(enemies[i].ship.x, enemies[i].ship.y, enemies[i].raio, enemies[i].ship.cor);
            }
        }
    }
}

void initGasolina() {
    item_gas.ativo = 1;
    item_gas.raio = 20 + rand() % 26; 

    item_gas.x = item_gas.raio + rand() % (int)(SCREEN_W - 80 - item_gas.raio * 2);
    item_gas.y = -(rand() % 500) - item_gas.raio; 
    item_gas.vel = 2; 
}

void checkGameOver(Hero *h, int *playing) {
    if (h->invulneravel_timer > 0) return;

    float nave_centro_x = h->ship.x;
    float nave_centro_y = h->ship.y + (HERO_H / 2.0); 
    float raio_heroi_justo = (HERO_W / 2.0) * 0.5; 

    for(int i = 0; i < NUM_ENEMIES; i++) {
        if(enemies[i].active) {
            float dx = nave_centro_x - enemies[i].ship.x;
            float dy = nave_centro_y - enemies[i].ship.y;
            float distancia = sqrt(dx*dx + dy*dy);
            float raio_meteoro_justo = enemies[i].raio * 0.5;

            if(distancia < (raio_heroi_justo + raio_meteoro_justo)) {
                h->vidas--; 
                
                if (h->vidas <= 0) {
                    *playing = 0; 
                } else {
                    h->invulneravel_timer = TEMPO_INVULNERAVEL;
                    initSingleEnemy(i); 
                }
                break;
            }
        }
    }
}

void checkCollisions(Hero *h) {
    if(h->ship.tiro.modo == TIRO_ATIVO) {
        h->score -= SCORE_PENALTY / FPS; 
        if(h->score < 0) h->score = 0; 

        for(int i = 0; i < NUM_ENEMIES; i++) {
            if(enemies[i].active) {
                float dx = h->ship.tiro.x - enemies[i].ship.x;
                float dy = h->ship.tiro.y - enemies[i].ship.y;
                float distancia = sqrt(dx*dx + dy*dy);

                if(distancia < (h->ship.tiro.raio + enemies[i].raio)) {
                    h->score += (50.0 / enemies[i].raio) * 10;
                    initSingleEnemy(i); 
                }
            }
        }
    }
}

void drawScenario(ALLEGRO_BITMAP *bg) {
    if(bg) {
        int img_w = al_get_bitmap_width(bg);
        int img_h = al_get_bitmap_height(bg);
        float escala_h = ((float)img_h * SCREEN_W) / img_w;

        al_draw_scaled_bitmap(bg, 0, 0, img_w, img_h, 0, background_y, SCREEN_W, escala_h, 0);
        al_draw_scaled_bitmap(bg, 0, 0, img_w, img_h, 0, background_y - escala_h, SCREEN_W, escala_h, ALLEGRO_FLIP_VERTICAL);

        background_y += 1.0; 
        if(background_y >= escala_h) {
            background_y = 0; 
        }
    } else {
        al_clear_to_color(BKG_COLOR);
    }
}

// Renderiza a interface do herói com os corações gigantes e a caixa cinza reduzida no tamanho exato do Score
void drawHero(Hero s) {
    int desenhar_nave = 1;
    if (s.invulneravel_timer > 0) {
        if (((int)(s.invulneravel_timer * 10) % 2) == 0) {
            desenhar_nave = 0;
        }
    }

    if (desenhar_nave) {
        if(nave) {
            al_draw_scaled_bitmap(nave,
                0, 0, al_get_bitmap_width(nave), al_get_bitmap_height(nave),
                s.ship.x - HERO_W/2, s.ship.y, HERO_W, HERO_H, 0);                                                               
        } else {
            al_draw_filled_triangle(s.ship.x, s.ship.y, s.ship.x - HERO_W/2, s.ship.y + HERO_H, s.ship.x + HERO_W/2, s.ship.y + HERO_H, s.ship.cor);
        }
    }

    // Caixa Lilás da Pontuação
    char score_txt[15];
    sprintf(score_txt, "Score: %d", (int)s.score);
    int text_w = al_get_text_width(FONT_24, score_txt);
    int box_w = text_w + 20; 
    int box_h = 35;
    int box_x = 20;  
    int box_y = 20;  

    al_draw_filled_rounded_rectangle(box_x, box_y, box_x + box_w, box_y + box_h, 6, 6, al_map_rgb(215, 185, 245));
    al_draw_text(FONT_24, al_map_rgb(75, 20, 110), box_x + (box_w / 2), box_y + 5, ALLEGRO_ALIGN_CENTRE, score_txt);

    // Caixinha Cinza reduzida ao máximo (altura 42 e largura 114) para se igualar ao Score
    int v_box_x = 20;
    int v_box_y = box_y + box_h + 8; 
    int v_box_w = 114; 
    int v_box_h = 42;  

    // Desenha o fundo Cinza Claro super compacto
    al_draw_filled_rounded_rectangle(v_box_x, v_box_y, v_box_x + v_box_w, v_box_y + v_box_h, 6, 6, al_map_rgb(200, 200, 200));

    int tam_coracao = 46; 
    int espacamento = 32; 

    for (int i = 0; i < VIDAS_MAX; i++) {
        int c_x = v_box_x + 6 + (i * espacamento);
        int c_y = v_box_y + (v_box_h / 2) - (tam_coracao / 2);

        if (i < s.vidas) {
            if (img_vidas) {
                al_draw_scaled_bitmap(img_vidas,
                    0, 0, al_get_bitmap_width(img_vidas), al_get_bitmap_height(img_vidas),
                    c_x, c_y, tam_coracao, tam_coracao, 0);
            } else {
                al_draw_filled_circle(c_x + tam_coracao/4, c_y + tam_coracao/4, tam_coracao/4, al_map_rgb(235, 40, 40));
                al_draw_filled_circle(c_x + 3*tam_coracao/4, c_y + tam_coracao/4, tam_coracao/4, al_map_rgb(235, 40, 40));
                al_draw_filled_triangle(c_x, c_y + tam_coracao/3, c_x + tam_coracao, c_y + tam_coracao/3, c_x + tam_coracao/2, c_y + tam_coracao, al_map_rgb(235, 40, 40));
            }
        } else {
            int b_tam = tam_coracao - 16; 
            int b_x = c_x + 8;
            int b_y = c_y + 8;
            al_draw_circle(b_x + b_tam/4, b_y + b_tam/4, b_tam/4, al_map_rgb(0, 0, 0), 2.5);
            al_draw_circle(b_x + 3*b_tam/4, b_y + b_tam/4, b_tam/4, al_map_rgb(0, 0, 0), 2.5);
            al_draw_triangle(b_x, b_y + b_tam/3, b_x + b_tam, b_y + b_tam/3, b_x + b_tam/2, b_y + b_tam, al_map_rgb(0, 0, 0), 2.5);
        }
    }

    if(s.ship.tiro.modo == TIRO_ATIVO) {
        al_draw_circle(s.ship.tiro.x, s.ship.tiro.y, s.ship.tiro.raio, s.ship.tiro.cor, BORDA_TIRO);
    }
}

void updateHero(Hero *s) {
    s->ship.x += s->dir_x * s->ship.vel;
    s->ship.y += s->dir_y * s->ship.vel;

    if(s->ship.x - HERO_W/2 < 0)           s->ship.x = HERO_W/2;
    if(s->ship.x + HERO_W/2 > SCREEN_W - 50) s->ship.x = SCREEN_W - 50 - HERO_W/2;
    if(s->ship.y < 0)                      s->ship.y = 0;
    if(s->ship.y + HERO_H > SCREEN_H)      s->ship.y = SCREEN_H - HERO_H;

    if (s->invulneravel_timer > 0) {
        s->invulneravel_timer -= 1.0 / FPS;
    }

    if(s->ship.tiro.modo != TIRO_ATIVO) {
        s->ship.tiro.x = s->ship.x;
        s->ship.tiro.y = s->ship.y;
    }
    else {
        if(s->ship.tiro.timer > 0)
            s->ship.tiro.timer -= 1.0/FPS;
        else
            initTiro(&s->ship); 
    }
}

void drawGameOverScreen(Hero h) {
    al_draw_filled_rectangle(0, 0, SCREEN_W, SCREEN_H, al_map_rgba(0, 0, 0, 195));
    al_draw_text(FONT_52, al_map_rgb(255, 60, 60), SCREEN_W / 2, SCREEN_H / 2 - 150, ALLEGRO_ALIGN_CENTRE, "GAME OVER");

    float px = SCREEN_W / 2;
    float py = SCREEN_H / 2 - 40;
    al_draw_filled_rounded_rectangle(px - 210, py - 5, px + 210, py + 45, 10, 10, al_map_rgb(215, 185, 245));
    
    char score_text[40];
    sprintf(score_text, "Pontuação Final: %d", (int)h.score); 
    al_draw_text(FONT_32, al_map_rgb(75, 20, 110), px, py, ALLEGRO_ALIGN_CENTRE, score_text);

    float bx = SCREEN_W / 2;
    float by = SCREEN_H / 2 + 50;
    al_draw_filled_rectangle(bx - 360, by - 5, bx + 360, by + 90, al_map_rgb(60, 60, 60));
    al_draw_text(FONT_32, al_map_rgb(215, 185, 245), bx, by + 5, ALLEGRO_ALIGN_CENTRE, "Pressione ESPAÇO para jogar novamente");
    al_draw_text(FONT_24, al_map_rgb(160, 160, 160), bx, by + 52, ALLEGRO_ALIGN_CENTRE, "ou pressione ESC / ENTER para sair");
    al_flip_display();
}

void updateCombustivel(Hero *h, int *playing) {
    h->combustivel -= TAXA_CONSUMO / FPS;
    if(h->combustivel <= 0) {
        h->combustivel = 0;
        *playing = 0; 
    }

    if(item_gas.ativo) {
        item_gas.y += item_gas.vel;
        if(item_gas.y > SCREEN_H) {
            initGasolina();
        }
    }
}

void checkGasColisao(Hero *h) {
    if(!item_gas.ativo) return;

    float dx = h->ship.x - item_gas.x;
    float dy = (h->ship.y + HERO_H/2) - item_gas.y;
    float distancia = sqrt(dx*dx + dy*dy);

    if(distancia < (HERO_W/2 + item_gas.raio)) {
        h->combustivel += 35.0; 
        if(h->combustivel > COMBUSTIVEL_MAX) h->combustivel = COMBUSTIVEL_MAX;
        initGasolina(); 
    }
}

void drawGasolinaEInterface(Hero h) {
    if(item_gas.ativo) {
        if(img_combustivel) {
            float diametro = item_gas.raio * 2;
            al_draw_scaled_bitmap(img_combustivel,
                0, 0, al_get_bitmap_width(img_combustivel), al_get_bitmap_height(img_combustivel),
                item_gas.x - item_gas.raio, item_gas.y - item_gas.raio, diametro, diametro, 0);
        } else {
            al_draw_filled_circle(item_gas.x, item_gas.y, item_gas.raio, al_map_rgb(235, 105, 20));
        }
    }

    float barra_x1 = SCREEN_W - 35;
    float barra_x2 = SCREEN_W - 15;
    float barra_y1 = SCREEN_H / 2 - 90; 
    float barra_y2 = SCREEN_H / 2 + 90; 
    float altura_max = 180.0;

    al_draw_filled_rectangle(barra_x1, barra_y1, barra_x2, barra_y2, al_map_rgb(50, 50, 50));
    float altura_atual = (h.combustivel / COMBUSTIVEL_MAX) * altura_max;
    
    ALLEGRO_COLOR cor_barra = al_map_rgb(235, 105, 20); 
    if(h.combustivel < 30) {
        cor_barra = al_map_rgb(220, 50, 50); 
    }

    al_draw_filled_rectangle(barra_x1, barra_y2 - altura_atual, barra_x2, barra_y2, cor_barra);
    al_draw_rectangle(barra_x1, barra_y1, barra_x2, barra_y2, al_map_rgb(255, 255, 255), 2);
    
    // Texto GÁS perfeitamente acentuado
    al_draw_text(FONT_18, al_map_rgb(255, 255, 255), barra_x1 - 15, barra_y1 - 25, ALLEGRO_ALIGN_LEFT, "GÁS");
}


int main(int argc, char **argv){
    setlocale(LC_ALL, "Portuguese");

    ALLEGRO_DISPLAY *display = NULL;
    ALLEGRO_EVENT_QUEUE *event_queue = NULL;
    ALLEGRO_TIMER *timer = NULL;
    
    if(!al_init()) return -1;
    if(!al_init_primitives_addon()) return -1;
    if(!al_init_image_addon()) return -1;
    
    timer = al_create_timer(1.0 / FPS);
    if(!timer) return -1;

    display = al_create_display(SCREEN_W, SCREEN_H);
    if(!display) {
        al_destroy_timer(timer);
        return -1;
    }

    if(!al_install_keyboard()) {
        al_destroy_display(display);
        al_destroy_timer(timer);
        return -1;
    }

    al_init_font_addon();
    if(!al_init_ttf_addon()) {
        al_destroy_display(display);
        al_destroy_timer(timer);
        return -1;
    }
    
    initGlobals();
    event_queue = al_create_event_queue();

    background = al_load_bitmap("background.png"); 
    nave = al_load_bitmap("nave.png"); 
    meteoro = al_load_bitmap("meteoro.png"); 
    img_combustivel = al_load_bitmap("combustivel.png");
    img_vidas = al_load_bitmap("vidas.png"); 

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    int jogar_novamente = 1;

    while(jogar_novamente) {
        Hero Hero;
        initHero(&Hero);
        initEnemies();
        initGasolina();

        al_start_timer(timer); 
        int playing = 1;       
        
        while(playing) {
            ALLEGRO_EVENT ev;
            al_wait_for_event(event_queue, &ev);

            if(ev.type == ALLEGRO_EVENT_TIMER) {
                // ORDEM CORRETA DE ATUALIZAÇÃO E DESENHO:
                updateHero(&Hero);        
                updateEnemies();
                updateCombustivel(&Hero, &playing);
                checkGasColisao(&Hero);
                checkGameOver(&Hero, &playing);       
                checkCollisions(&Hero);

                // Primeiro limpa e desenha o fundo
                drawScenario(background); 
                // Depois os meteoros
                drawEnemies();
                // E por cima de tudo os elementos fixos do HUD e do Jogador
                drawHero(Hero);           
                drawGasolinaEInterface(Hero);

                al_flip_display();
            }
            else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                playing = 0;
                jogar_novamente = 0; 
            }
            else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                switch(ev.keyboard.keycode) {
                    case ALLEGRO_KEY_W: Hero.dir_y--; break;
                    case ALLEGRO_KEY_S: Hero.dir_y++; break;
                    case ALLEGRO_KEY_A: Hero.dir_x--; break;
                    case ALLEGRO_KEY_D: Hero.dir_x++; break;  
                    case ALLEGRO_KEY_SPACE:
                        if(Hero.ship.tiro.modo == TIRO_INATIVO) {
                            Hero.ship.tiro.modo = TIRO_ATIVO;
                            Hero.ship.tiro.raio = RAIO_TIRO;
                        }
                    break;
                }
            }           
            else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
                switch(ev.keyboard.keycode) {
                    case ALLEGRO_KEY_W: Hero.dir_y++; break;
                    case ALLEGRO_KEY_S: Hero.dir_y--; break;
                    case ALLEGRO_KEY_A: Hero.dir_x++; break;
                    case ALLEGRO_KEY_D: Hero.dir_x--; break;  
                }           
            }
        } 

        al_stop_timer(timer); 

        if (jogar_novamente) {
            drawGameOverScreen(Hero); 

            int aguardando_saida = 1;
            while(aguardando_saida) {
                ALLEGRO_EVENT ev_morte;
                al_wait_for_event(event_queue, &ev_morte);

                if(ev_morte.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                    aguardando_saida = 0;
                    jogar_novamente = 0; 
                }
                if(ev_morte.type == ALLEGRO_EVENT_KEY_DOWN) {
                    if(ev_morte.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                        aguardando_saida = 0; 
                    }
                    if(ev_morte.keyboard.keycode == ALLEGRO_KEY_ESCAPE || 
                       ev_morte.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                        aguardando_saida = 0;
                        jogar_novamente = 0; 
                    }
                }
            }
        }
    } 
 
    al_destroy_font(FONT_52);
    al_destroy_font(FONT_32);
    al_destroy_font(FONT_24);
    al_destroy_font(FONT_18); 
    al_destroy_timer(timer);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    al_destroy_bitmap(background);
    al_destroy_bitmap(nave);
    al_destroy_bitmap(meteoro);
    al_destroy_bitmap(img_combustivel); 
    if(img_vidas) al_destroy_bitmap(img_vidas); 
 
    return 0; 
}