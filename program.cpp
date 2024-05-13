#include "splashkit.h"
#include <cstdlib> // Include for random number generation

using std::to_string;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 50;
const int NUM_TILES_X = SCREEN_WIDTH / TILE_SIZE;
const int NUM_TILES_Y = SCREEN_HEIGHT / TILE_SIZE;
const int COINS_TO_WIN = 15;
const int MAX_MOBS = 5; // Maximum number of mobs on the board at all times
const int TICK_SPEED = 10; // Every 10 ticks a new mob spawns
const int WATER_SPAWN_CHANCE = 3; // Chance for water to spawn (out of 10)
const int MAX_AIR = 100; // Maximum air the player can have
const int AIR_LOSS_RATE = 1; // Rate of air loss per second
const int DROWN_THRESHOLD = 10; // Time threshold (in seconds) before player starts drowning

enum TileType { GRASS, WATER, WALL, DOOR };
enum GameState { PLAYING, GAME_OVER };

struct Tile {
    TileType type;
    bool traversable;
    int x, y;
};

struct Mob {
    int x, y;
    int health;
    int damage;
    int move_direction; // 0: up, 1: down, 2: left, 3: right
};

struct Player {
    int x, y;
    int health;
    int damage;
    bool has_key;
    int air;
};

struct Game {
    Player player;
    Tile world[NUM_TILES_X][NUM_TILES_Y];
    Mob mobs[MAX_MOBS];
    int num_mobs;
    GameState state;
    int tick_counter; // Counter for tick system
};

Game game;

void setup(Game &game);
void draw_world(const Game &game);
void draw_player(const Game &game);
void draw_mobs(const Game &game);
void draw_health(const Game &game);
void draw_air(const Game &game);
void draw_score(const Game &game);
void handle_input(Game &game);
bool is_traversable(int x, int y);
void move_player(Game &game, int dx, int dy);
void update_game_state(Game &game);
void spawn_mobs(Game &game);
void move_mobs(Game &game);
void draw_game_over();
void check_random_event(Game &game);

int main() {
    open_window("Tile-Based RPG", SCREEN_WIDTH, SCREEN_HEIGHT);
    setup(game);

    do {
        process_events();
        clear_screen(COLOR_WHITE);

        if (game.state == PLAYING) {
            handle_input(game);
            draw_world(game);
            draw_mobs(game);
            draw_player(game);
            draw_health(game);
            draw_air(game);
            draw_score(game); // Draw the player's score
            update_game_state(game);
            // Increment the tick counter
            game.tick_counter++;
            // Spawn new mobs every TICK_SPEED ticks
            if (game.tick_counter >= TICK_SPEED) {
                spawn_mobs(game);
                game.tick_counter = 0; // Reset the tick counter
            }
            // Move mobs
            move_mobs(game);
            
        } else {
            draw_game_over();
            break; // Exit loop when game is over
        }

        refresh_screen(60);
    } while (!window_close_requested("Tile-Based RPG"));

    return 0;
}

void setup(Game &game) {
    game.player.x = rnd(NUM_TILES_X) * TILE_SIZE;
    game.player.y = rnd(NUM_TILES_Y) * TILE_SIZE;
    game.player.health = 10;
    game.player.damage = 10;
    game.player.has_key = false;
    game.player.air = MAX_AIR;
    game.num_mobs = 0;
    game.state = PLAYING;
    game.tick_counter = 0; // Initialize tick counter

    for (int i = 0; i < NUM_TILES_X; ++i) {
        for (int j = 0; j < NUM_TILES_Y; ++j) {
            game.world[i][j].x = i * TILE_SIZE;
            game.world[i][j].y = j * TILE_SIZE;
            // Set tile type and traversable attribute
            if (i == 0 || i == NUM_TILES_X - 1 || j == 0 || j == NUM_TILES_Y - 1) {
                game.world[i][j].type = WALL;
                game.world[i][j].traversable = false;
            