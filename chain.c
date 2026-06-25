#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>   
#include <allegro5/allegro_acodec.h>  
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <locale.h> 

// DEFINIÇÕES DE CONSTANTES DO JOGO

#define NUM_ENEMIES 15          // Número máximo de meteoros na tela
#define TIRO_INATIVO 0          // Estado do tiro quando não foi disparado
#define TIRO_ATIVO 1            // Estado do tiro quando está na tela
#define RAIO_TIRO 100           // Raio do círculo do tiro especial
#define TEMPO_TIRO 2.0          // Duração do tiro na tela (em segundos)
#define BORDA_TIRO 4            // Espessura da linha do tiro
#define SCORE_PENALTY 0.3       // Penalidade de pontos enquanto o tiro está ativo
#define COMBUSTIVEL_MAX 100.0   // Capacidade máxima do tanque de gás
#define TAXA_CONSUMO 8.0        // Velocidade com que o combustível gasta
#define VIDAS_MAX 3             // Quantidade máxima de vidas (corações)
#define TEMPO_INVULNERAVEL 2.0  // Tempo de piscar sem morrer após tomar dano

const float FPS = 100;          // Atualizações por segundo do jogo (Frames Per Second)

// Dimensões da tela e da nave do jogador
const int SCREEN_W = 960;
const int SCREEN_H = 540;
const int HERO_W = 70;
const int HERO_H = 90;

// ==========================================
// VARIÁVEIS GLOBAIS (IMAGENS, SONS E FONTES)
// ==========================================
ALLEGRO_BITMAP *background = NULL;     // Imagem do espaço (fundo)
ALLEGRO_BITMAP *nave = NULL;           // Imagem da nave do jogador
ALLEGRO_BITMAP *meteoro = NULL;        // Imagem do meteoro (inimigo)
ALLEGRO_BITMAP *img_combustivel = NULL;// Imagem do item de combustível
ALLEGRO_BITMAP *img_vidas = NULL;      // Imagem do coração de vida
float background_y = 0;                // Posição Y para fazer o fundo rolar
ALLEGRO_COLOR BKG_COLOR;               // Cor reserva caso a imagem de fundo suma

ALLEGRO_SAMPLE *musica_fundo = NULL;             // Arquivo de áudio carregado
ALLEGRO_SAMPLE_INSTANCE *instancia_musica = NULL;// Instância para controlar o loop do som

// Declaração dos tamanhos de fontes de texto
ALLEGRO_FONT *FONT_52; 
ALLEGRO_FONT *FONT_32; 
ALLEGRO_FONT *FONT_24; 
ALLEGRO_FONT *FONT_18; 

// ==========================================
// ESTRUTURAS DE DADOS (STRUCTS)
// ==========================================

// Estrutura para controlar o Tiro Especial
typedef struct Tiro {
    int x, y;           // Coordenadas do centro do tiro
    float raio;         // Tamanho atual do raio do tiro
    int modo;           // Se está ATIVO ou INATIVO
    float timer;        // Contador de tempo para sumir da tela
    ALLEGRO_COLOR cor;  // Cor do círculo do tiro
} Tiro;

// Estrutura genérica para naves/objetos móveis
typedef struct Ship {
    int x, y;           // Posição na tela
    int vel;            // Velocidade de movimento
    ALLEGRO_COLOR cor;  // Cor reserva
    Tiro tiro;          // Cada nave possui um sistema de tiro atrelado
} Ship;

// Estrutura do Jogador (Herói)
typedef struct Hero {
    Ship ship;                  // Dados básicos de movimento e posição
    int dir_x;                  // Direção horizontal (-1 esquerda, 1 direita, 0 parado)
    int dir_y;                  // Direção vertical (-1 cima, 1 baixo, 0 parado)
    float score;                // Pontuação atual do jogador
    float combustivel;          // Nível atual de combustível
    int vidas;                  // Quantidade atual de vidas
    float invulneravel_timer;   // Tempo restante de imunidade após bater
} Hero;

// Estrutura dos Inimigos (Meteoros)
typedef struct Enemy {
    Ship ship;      // Posição e velocidade
    float raio;     // Raio do meteoro (determina o tamanho dele)
    int active;     // Se o meteoro está ativo na tela ou não
    int vel;        // Velocidade individual de queda
} Enemy; 

Enemy enemies[NUM_ENEMIES]; // Vetor com todos os meteoros do jogo

// Estrutura do item de Combustível (Gasolina)
typedef struct Gasolina {
    float x, y;     // Posição na tela
    float vel;      // Velocidade de queda do item
    float raio;     // Tamanho da área de colisão do item
    int ativo;      // Se o item está visível para coleta
} Gasolina;

Gasolina item_gas; // Instância global do item de gasolina


// FUNÇÕES DE INICIALIZAÇÃO E CONTROLE

// Reseta o estado do tiro para o padrão (guardado e pronto)
void initTiro(Ship *s) {
    s->tiro.x = s->x;
    s->tiro.y = s->y;
    s->tiro.raio = 3;
    s->tiro.cor = s->cor;
    s->tiro.timer = TEMPO_TIRO;
    s->tiro.modo = TIRO_INATIVO;
}

// Inicializa cores de segurança e carrega os arquivos de fontes TTF/OTF
void initGlobals() {
    BKG_COLOR = al_map_rgb(10, 10, 10);
    FONT_52 = al_load_font("Nowster Cute.otf", 52, 0);   
    FONT_32 = al_load_font("Hey Comic.otf", 32, 0);   
    FONT_24 = al_load_font("arial.ttf", 24, 0); 
    FONT_18 = al_load_font("arial.ttf", 18, 0); 
}

// Define os status iniciais do herói quando o jogo começa/reinicia
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

// Inicializa ou redistribui um único meteoro no topo da tela com tamanho aleatório
void initSingleEnemy(int i) {
    enemies[i].active = 1;
    enemies[i].raio = 60 + rand() % 26; 
    enemies[i].vel = 1 + rand() % 3;
    enemies[i].ship.x = enemies[i].raio + rand() % (int)(SCREEN_W - enemies[i].raio * 2 - 60);
    enemies[i].ship.y = -(rand() % 200) - enemies[i].raio; // Começa acima da tela para surgir suavemente
    enemies[i].ship.cor = al_map_rgb(150 + rand()%106, rand()%100, rand()%100);
}

// Inicializa o bando completo de meteoros no começo do jogo
void initEnemies() {
    for(int i = 0; i < NUM_ENEMIES; i++) {
        initSingleEnemy(i);
    }
}

// Atualiza a posição de queda dos meteoros (fica mais rápido conforme o score sobe)
void updateEnemies(float score) {
    float bonus_velocidade = score / 200.0; 
    for(int i = 0; i < NUM_ENEMIES; i++) {
        if(enemies[i].active) {
            enemies[i].ship.y += (enemies[i].vel + bonus_velocidade); 
            // Se o meteoro passou do limite inferior da tela, ele ressurge no topo
            if(enemies[i].active && enemies[i].ship.y - enemies[i].raio > SCREEN_H) {
                initSingleEnemy(i);
            }
        }
    }
}

// Desenha as imagens dos meteoros na tela, ou círculos se a imagem sumir
void drawEnemies() {
    for(int i = 0; i < NUM_ENEMIES; i++) {
        if(enemies[i].active) {
            if(meteoro) {
                float diametro = enemies[i].raio * 2;
                al_draw_scaled_bitmap(meteoro,
                    0, 0, al_get_bitmap_width(meteoro), al_get_bitmap_height(meteoro),
                    enemies[i].ship.x - enemies[i].raio, enemies[i].ship.y - enemies[i].raio,       
                    diametro, diametro, 0);
            } else {
                al_draw_filled_circle(enemies[i].ship.x, enemies[i].ship.y, enemies[i].raio, enemies[i].ship.cor);
            }
        }
    }
}

// Inicializa o galão de gasolina no topo da tela de forma aleatória
void initGasolina() {
    item_gas.ativo = 1;
    item_gas.raio = 20 + rand() % 26; 
    item_gas.x = item_gas.raio + rand() % (int)(SCREEN_W - 80 - item_gas.raio * 2);
    item_gas.y = -(rand() % 500) - item_gas.raio; 
    item_gas.vel = 2; 
}

// ==========================================
// SISTEMA DE COLISÕES E LÓGICA DE DANOS
// ==========================================

// Verifica se a nave bateu em algum meteoro (Gera perda de vida)
void checkGameOver(Hero *h, int *playing) {
    if (h->invulneravel_timer > 0) return; // Se estiver imune (piscando), ignora colisões

    float nave_centro_x = h->ship.x;
    float nave_centro_y = h->ship.y + (HERO_H / 2.0); 
    float raio_heroi_justo = (HERO_W / 2.0) * 0.5; // Ajuste para colisão mais justa na nave

    for(int i = 0; i < NUM_ENEMIES; i++) {
        if(enemies[i].active) {
            float dx = nave_centro_x - enemies[i].ship.x;
            float dy = nave_centro_y - enemies[i].ship.y;
            float distancia = sqrt(dx*dx + dy*dy);
            float raio_meteoro_justo = enemies[i].raio * 0.5;

            // Se a distância for menor que a soma dos raios, houve colisão!
            if(distancia < (raio_heroi_justo + raio_meteoro_justo)) {
                h->vidas--; // Perde uma vida
                if (h->vidas <= 0) {
                    *playing = 0; // Se zerar as vidas, define fim de jogo
                } else {
                    h->invulneravel_timer = TEMPO_INVULNERAVEL; // Ativa o tempo piscando
                    initSingleEnemy(i); // Destrói o meteoro que bateu nele
                }
                break;
            }
        }
    }
}

// Verifica se o tiro do jogador atingiu algum meteoro para ganhar pontos
void checkCollisions(Hero *h) {
    if(h->ship.tiro.modo == TIRO_ATIVO) {
        h->score -= SCORE_PENALTY / FPS; // Desconta um tiquinho de pontos por segundo com tiro ligado
        if(h->score < 0) h->score = 0; 

        for(int i = 0; i < NUM_ENEMIES; i++) {
            if(enemies[i].active) {
                float dx = h->ship.tiro.x - enemies[i].ship.x;
                float dy = h->ship.tiro.y - enemies[i].ship.y;
                float distancia = sqrt(dx*dx + dy*dy);

                // Se o círculo do tiro encostar no meteoro, o meteoro explode
                if(distancia < (h->ship.tiro.raio + enemies[i].raio)) {
                    h->score += (50.0 / enemies[i].raio) * 10; // Mais pontos para meteoros menores
                    initSingleEnemy(i); // Resubmete o meteoro explodido
                }
            }
        }
    }
}

// ==========================================
// RENDERIZAÇÃO GRÁFICA (DESENHOS)
// ==========================================

// Desenha o cenário de fundo com efeito Parallax (rolagem infinita)
void drawScenario(ALLEGRO_BITMAP *bg) {
    if(bg) {
        int img_w = al_get_bitmap_width(bg);
        int img_h = al_get_bitmap_height(bg);
        float escala_h = ((float)img_h * SCREEN_W) / img_w;

        // Desenha duas cópias do fundo se alternando verticalmente para simular movimento infinito
        al_draw_scaled_bitmap(bg, 0, 0, img_w, img_h, 0, background_y, SCREEN_W, escala_h, 0);
        al_draw_scaled_bitmap(bg, 0, 0, img_w, img_h, 0, background_y - escala_h, SCREEN_W, escala_h, ALLEGRO_FLIP_VERTICAL);

        background_y += 1.0; 
        if(background_y >= escala_h) {
            background_y = 0; 
        }
    } else {
        al_clear_to_color(BKG_COLOR); // Fundo preto de emergência
    }
}

// Desenha a nave, interface de pontuação e o sistema de corações de vida
void drawHero(Hero s) {
    int desenhar_nave = 1;
    // Sistema para fazer a nave piscar se estiver invulnerável
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

        // Desenha um escudo azul ao redor se estiver invulnerável
        if (s.invulneravel_timer > 0) {
            al_draw_circle(s.ship.x, s.ship.y + (HERO_H / 2.0), HERO_W * 0.8, al_map_rgb(0, 180, 255), 3);
        }
    }

    // Caixa Lilás da Pontuação (Score)
    char score_txt[15];
    sprintf(score_txt, "Score: %d", (int)s.score);
    int text_w = al_get_text_width(FONT_24, score_txt);
    int box_w = text_w + 20; 
    int box_h = 35;
    int box_x = 20;  
    int box_y = 20;  

    al_draw_filled_rounded_rectangle(box_x, box_y, box_x + box_w, box_y + box_h, 6, 6, al_map_rgb(215, 185, 245));
    al_draw_text(FONT_24, al_map_rgb(75, 20, 110), box_x + (box_w / 2), box_y + 5, ALLEGRO_ALIGN_CENTRE, score_txt);

    // Caixinha Cinza das Vidas (Largura e altura ampliadas para os novos corações maiores)
    int v_box_x = 20;
    int v_box_y = box_y + box_h + 8; 
    int v_box_w = 155; // Aumentado de 125 para 155 para dar folga lateral
    int v_box_h = 56;  // Aumentado de 46 para 56 para manter a folga em cima e embaixo

    al_draw_filled_rounded_rectangle(v_box_x, v_box_y, v_box_x + v_box_w, v_box_y + v_box_h, 6, 6, al_map_rgb(200, 200, 200));

    // Tamanho ideal ampliado para 42px para preencher a caixinha
    int tam_coracao = 42; 
    int espacamento = 45; // Espaço proporcional entre o início de cada coração

    for (int i = 0; i < VIDAS_MAX; i++) {
        int c_x = v_box_x + 12 + (i * espacamento);
        int c_y = v_box_y + (v_box_h / 2) - (tam_coracao / 2);

        if (i < s.vidas) {
            // Vida ativa: Desenha a imagem do coração vermelho normal ampliada
            if (img_vidas) {
                al_draw_scaled_bitmap(img_vidas, 0, 0, al_get_bitmap_width(img_vidas), al_get_bitmap_height(img_vidas), c_x, c_y, tam_coracao, tam_coracao, 0);
            } else {
                al_draw_filled_circle(c_x + tam_coracao/4, c_y + tam_coracao/4, tam_coracao/4, al_map_rgb(235, 40, 40));
                al_draw_filled_circle(c_x + 3*tam_coracao/4, c_y + tam_coracao/4, tam_coracao/4, al_map_rgb(235, 40, 40));
                al_draw_filled_triangle(c_x, c_y + tam_coracao/3, c_x + tam_coracao, c_y + tam_coracao/3, c_x + tam_coracao/2, c_y + tam_coracao, al_map_rgb(235, 40, 40));
            }
        } else {
            // Vida perdida: CORRIGIDO de al_draw_scaled_tinted_bitmap para al_draw_tinted_scaled_bitmap
            if (img_vidas) {
                al_draw_tinted_scaled_bitmap(img_vidas, al_map_rgba(35, 35, 35, 130), 0, 0, al_get_bitmap_width(img_vidas), al_get_bitmap_height(img_vidas), c_x, c_y, tam_coracao, tam_coracao, 0);
            } else {
                al_draw_circle(c_x + tam_coracao/4, c_y + tam_coracao/4, tam_coracao/4, al_map_rgb(0, 0, 0), 2);
                al_draw_circle(c_x + 3*tam_coracao/4, c_y + tam_coracao/4, tam_coracao/4, al_map_rgb(0, 0, 0), 2);
                al_draw_triangle(c_x, c_y + tam_coracao/3, c_x + tam_coracao, c_y + tam_coracao/3, c_x + tam_coracao/2, c_y + tam_coracao, al_map_rgb(0, 0, 0), 2);
            }
        }
    }

    // Desenha o círculo do tiro especial se estiver em execução
    if(s.ship.tiro.modo == TIRO_ATIVO) {
        al_draw_circle(s.ship.tiro.x, s.ship.tiro.y, s.ship.tiro.raio, s.ship.tiro.cor, BORDA_TIRO);
    }
}

// Movimenta o jogador na tela e impede que ele saia das bordas
void updateHero(Hero *s) {
    s->ship.x += s->dir_x * s->ship.vel;
    s->ship.y += s->dir_y * s->ship.vel;

    // Barreiras invisíveis para prender a nave dentro do jogo
    if(s->ship.x - HERO_W/2 < 0)           s->ship.x = HERO_W/2;
    if(s->ship.x + HERO_W/2 > SCREEN_W - 50) s->ship.x = SCREEN_W - 50 - HERO_W/2;
    if(s->ship.y < 0)                      s->ship.y = 0;
    if(s->ship.y + HERO_H > SCREEN_H)      s->ship.y = SCREEN_H - HERO_H;

    // Diminui o tempo restante de imunidade a cada frame
    if (s->invulneravel_timer > 0) {
        s->invulneravel_timer -= 1.0 / FPS;
    }

    // Atualiza posição ou tempo do tiro ativo
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

// Renderiza o menu escurecido de fim de jogo (Game Over)
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

// Consome o combustível da nave e movimenta os itens de gás caindo
void updateCombustivel(Hero *h, int *playing) {
    h->combustivel -= TAXA_CONSUMO / FPS;
    if(h->combustivel <= 0) {
        h->combustivel = 0;
        *playing = 0; // Fim de jogo por pane seca
    }

    if(item_gas.ativo) {
        item_gas.y += item_gas.vel;
        if(item_gas.y > SCREEN_H) {
            initGasolina();
        }
    }
}

// Verifica se o jogador coletou um galão de combustível
void checkGasColisao(Hero *h) {
    if(!item_gas.ativo) return;

    float dx = h->ship.x - item_gas.x;
    float dy = (h->ship.y + HERO_H/2) - item_gas.y;
    float distancia = sqrt(dx*dx + dy*dy);

    if(distancia < (HERO_W/2 + item_gas.raio)) {
        h->combustivel += 35.0; // Restaura combustível
        if(h->combustivel > COMBUSTIVEL_MAX) h->combustivel = COMBUSTIVEL_MAX;
        initGasolina(); // Gera novo galão no topo
    }
}

// Desenha o galão de gasolina e a barra lateral de combustível (termômetro)
void drawGasolinaEInterface(Hero h) {
    if(item_gas.ativo) {
        if(img_combustivel) {
            float diametro = item_gas.raio * 2;
            al_draw_scaled_bitmap(img_combustivel, 0, 0, al_get_bitmap_width(img_combustivel), al_get_bitmap_height(img_combustivel), item_gas.x - item_gas.raio, item_gas.y - item_gas.raio, diametro, diametro, 0);
        } else {
            al_draw_filled_circle(item_gas.x, item_gas.y, item_gas.raio, al_map_rgb(235, 105, 20)); // CORRIGIDO de 'orange' para 20
        }
    }

    // Desenho da barra de progresso no canto direito da tela
    float barra_x1 = SCREEN_W - 35;
    float barra_x2 = SCREEN_W - 15;
    float barra_y1 = SCREEN_H / 2 - 90; 
    float barra_y2 = SCREEN_H / 2 + 90; 
    float altura_max = 180.0;

    al_draw_filled_rectangle(barra_x1, barra_y1, barra_x2, barra_y2, al_map_rgb(50, 50, 50));
    float altura_atual = (h.combustivel / COMBUSTIVEL_MAX) * altura_max;
    
    ALLEGRO_COLOR cor_barra = al_map_rgb(235, 105, 20); 
    if(h.combustivel < 30) {
        cor_barra = al_map_rgb(220, 50, 50); // Fica vermelha em estado crítico
    }

    al_draw_filled_rectangle(barra_x1, barra_y2 - altura_atual, barra_x2, barra_y2, cor_barra);
    al_draw_rectangle(barra_x1, barra_y1, barra_x2, barra_y2, al_map_rgb(255, 255, 255), 2);
    al_draw_text(FONT_18, al_map_rgb(255, 255, 255), barra_x1 - 15, barra_y1 - 25, ALLEGRO_ALIGN_LEFT, "GÁS");
}

// ==========================================
// FUNÇÃO PRINCIPAL DO SISTEMA (MAIN)
// ==========================================
int main(int argc, char **argv){
    setlocale(LC_ALL, "Portuguese"); // Configura o terminal para ler acentos em português

    ALLEGRO_DISPLAY *display = NULL;
    ALLEGRO_EVENT_QUEUE *event_queue = NULL;
    ALLEGRO_TIMER *timer = NULL;
    
    // Inicialização dos Addons obrigatórios do Allegro 5
    if(!al_init()) return -1;
    if(!al_init_primitives_addon()) return -1;
    if(!al_init_image_addon()) return -1;
    
    // Inicialização do sistema de Áudio
    if(!al_install_audio()) return -1;
    if(!al_init_acodec_addon()) return -1;
    if(!al_reserve_samples(8)) return -1; 

    // Criação do Timer principal com base na taxa de FPS
    timer = al_create_timer(1.0 / FPS);
    if(!timer) return -1;

    // Criação da janela gráfica
    display = al_create_display(SCREEN_W, SCREEN_H);
    if(!display) {
        al_destroy_timer(timer);
        return -1;
    }

    // Inicialização do Teclado
    if(!al_install_keyboard()) {
        al_destroy_display(display);
        al_destroy_timer(timer);
        return -1;
    }

    // Inicialização de fontes de texto ttf
    al_init_font_addon();
    if(!al_init_ttf_addon()) {
        al_destroy_display(display);
        al_destroy_timer(timer);
        return -1;
    }
    
    initGlobals();
    event_queue = al_create_event_queue();

    // Carregamento de imagens externas presentes na pasta do projeto
    background = al_load_bitmap("background.png"); 
    nave = al_load_bitmap("nave.png"); 
    meteoro = al_load_bitmap("meteoro.png"); 
    img_combustivel = al_load_bitmap("combustivel.png");
    img_vidas = al_load_bitmap("vidas.png"); 

    // Sistema de segurança dupla para o som: Tenta carregar .wav nativo, se falhar puxa o .mp3 original
    musica_fundo = al_load_sample("musica.wav");
    if(!musica_fundo) {
        musica_fundo = al_load_sample("verzand-intergalactic-space-23014.mp3");
    }

    // Se o Allegro localizou e abriu a música com sucesso, inicia o loop infinito
    if(musica_fundo) {
        instancia_musica = al_create_sample_instance(musica_fundo);
        al_set_sample_instance_playmode(instancia_musica, ALLEGRO_PLAYMODE_LOOP);
        al_attach_sample_instance_to_mixer(instancia_musica, al_get_default_mixer());
        al_play_sample_instance(instancia_musica);
    }

    // Registro das fontes geradoras de eventos na fila global
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    int jogar_novamente = 1;

    // Loop de controle de reinicialização (Enquanto o usuário quiser jogar de novo)
    while(jogar_novamente) {
        Hero Hero;
        initHero(&Hero);
        initEnemies();
        initGasolina();

        al_start_timer(timer); 
        int playing = 1;       
        
        // Loop principal da partida ativa
        while(playing) {
            ALLEGRO_EVENT ev;
            al_wait_for_event(event_queue, &ev);

            // Bloco disparado a cada tick do relógio de FPS
            if(ev.type == ALLEGRO_EVENT_TIMER) {
                updateHero(&Hero);        
                updateEnemies(Hero.score); 
                updateCombustivel(&Hero, &playing);
                checkGasColisao(&Hero);
                checkGameOver(&Hero, &playing);       
                checkCollisions(&Hero);

                // Ordem de desenho (Camadas: fundo -> objetos -> jogador -> interface)
                drawScenario(background); 
                drawEnemies();
                drawHero(Hero);           
                drawGasolinaEInterface(Hero);

                al_flip_display(); // Atualiza a tela gráfica
            }
            // Evento disparado ao clicar no "X" para fechar a janela
            else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                playing = 0;
                jogar_novamente = 0; 
            }
            // Evento disparado ao pressionar uma tecla
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
            // Evento disparado ao soltar uma tecla
            else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
                switch(ev.keyboard.keycode) {
                    case ALLEGRO_KEY_W: Hero.dir_y++; break;
                    case ALLEGRO_KEY_S: Hero.dir_y--; break;
                    case ALLEGRO_KEY_A: Hero.dir_x++; break;
                    case ALLEGRO_KEY_D: Hero.dir_x--; break;  
                }           
            }
        } 

        al_stop_timer(timer); // Pausa o timer do jogo durante a tela de fim de jogo

        // Bloco de Menu de Pós-Morte
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
                        aguardando_saida = 0; // Reinicia a partida limpando os status
                    }
                    if(ev_morte.keyboard.keycode == ALLEGRO_KEY_ESCAPE || 
                       ev_morte.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                        aguardando_saida = 0;
                        jogar_novamente = 0; // Sai do programa em definitivo
                    }
                }
            }
        }
    } 
 
    // Finalização de ponteiros e liberação de memória RAM (Desalocação obrigatória)
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
 
    if(musica_fundo) al_destroy_sample(musica_fundo);
    if(instancia_musica) al_destroy_sample_instance(instancia_musica);

    return 0; 
}