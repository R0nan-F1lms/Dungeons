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

enum TileType { GRASS, WATER, DESERT };
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
};

struct Player {
    int x, y;
    int health;
    int damage;
    bool has_key;
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
void draw_score(const Game &game);
void handle_input(Game &game);
bool is_traversable(int x, int y);
void move_player(Game &game, int dx, int dy);
void update_game_state(Game &game);
void spawn_mobs(Game &game);
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
            draw_score(game); // Draw the player's score
            update_game_state(game);
            // Increment the tick counter
            game.tick_counter++;
            // Spawn new mobs every TICK_SPEED ticks
            if (game.tick_counter >= TICK_SPEED) {
                spawn_mobs(game);
                game.tick_counter = 0; // Reset the tick counter
            }
            
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
    game.player.health = 100;
    game.player.damage = 10;
    game.player.has_key = false;
    game.num_mobs = 0;
    game.state = PLAYING;
    game.tick_counter = 0; // Initialize tick counter

    for (int i = 0; i < NUM_TILES_X; ++i) {
        for (int j = 0; j < NUM_TILES_Y; ++j) {
            game.world[i][j].x = i * TILE_SIZE;
            game.world[i][j].y = j * TILE_SIZE;
            game.world[i][j].type = static_cast<TileType>(rnd(3));
            // Set traversable attribute based on tile type
            game.world[i][j].traversable = (game.world[i][j].type != WATER);
        }
    }

    spawn_mobs(game);
}

void draw_world(const Game &game) {
    for (int i = 0; i < NUM_TILES_X; ++i) {
        for (int j = 0; j < NUM_TILES_Y; ++j) {
            switch (game.world[i][j].type) {
                case GRASS:
                    fill_rectangle(COLOR_GREEN, game.world[i][j].x, game.world[i][j].y, TILE_SIZE, TILE_SIZE);
                    break;
                case WATER:
                    fill_rectangle(COLOR_BLUE, game.world[i][j].x, game.world[i][j].y, TILE_SIZE, TILE_SIZE);
                    break;
                case DESERT:
                    fill_rectangle(COLOR_YELLOW, game.world[i][j].x, game.world[i][j].y, TILE_SIZE, TILE_SIZE);
                    break;
            }
        }
    }
}

void draw_mobs(const Game &game) {
    for (int i = 0; i < game.num_mobs; ++i) {
        fill_circle(COLOR_GRAY, game.mobs[i].x + TILE_SIZE / 2, game.mobs[i].y + TILE_SIZE / 2, TILE_SIZE / 4);
    }
}

void draw_player(const Game &game) {
    fill_circle(COLOR_RED, game.player.x + TILE_SIZE / 2, game.player.y + TILE_SIZE / 2, TILE_SIZE / 4);
}

void draw_score(const Game &game) {
    // Convert the player's health and coin count to strings
    string health_text = "Health: " + to_string(game.player.health);
    //string key_text = "Has Key: " + (game.player.has_key ? "Yes" : "No");
    // Draw the health and key status on the screen
    draw_text(health_text, COLOR_BLACK, 10, 10);
    //draw_text(key_text, COLOR_BLACK, 10, 30);
}

void handle_input(Game &game) {
    if (key_typed(D_KEY)) {
        move_player(game, TILE_SIZE, 0);
    } else if (key_typed(A_KEY)) {
        move_player(game, -TILE_SIZE, 0);
    } else if (key_typed(W_KEY)) {
        move_player(game, 0, -TILE_SIZE);
    } else if (key_typed(S_KEY)) {
        move_player(game, 0, TILE_SIZE);
    } else if (key_typed(ESCAPE_KEY)) {
        draw_game_over();
    }
}

bool is_traversable(int x, int y) {
    if (x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return false;
    return game.world[x / TILE_SIZE][y / TILE_SIZE].traversable;
}

void move_player(Game &game, int dx, int dy) {
    int new_x = game.player.x + dx;
    int new_y = game.player.y + dy;

    if (is_traversable(new_x, new_y)) {
        game.player.x = new_x;
        game.player.y = new_y;
        // Check for mob collision
        for (int i = 0; i < game.num_mobs; ++i) {
            if (game.mobs[i].x == game.player.x && game.mobs[i].y == game.player.y) {
                // Decrease player's health when colliding with a mob
                game.player.health -= game.mobs[i].damage;
                // Remove the mob from the game
                for (int j = i; j < game.num_mobs - 1; ++j) {
                    game.mobs[j] = game.mobs[j + 1];
                }
                game.num_mobs--;
                break; // Stop checking for mob collisions once one is found
            }
        }
    }
}

void update_game_state(Game &game) {
    // Check if player's health drops to zero
    if (game.player.health <= 0) {
        game.state = GAME_OVER;
    }
}

void spawn_mobs(Game &game) {
    // Spawn new mobs up to MAX_MOBS
    for (int i = game.num_mobs; i < MAX_MOBS; ++i) {
        int x = rnd(NUM_TILES_X) * TILE_SIZE;
        int y = rnd(NUM_TILES_Y) * TILE_SIZE;

        // Check if the tile is traversable and not occupied by the player or another mob
        bool traversable = is_traversable(x, y);
        bool occupied_by_player = (x == game.player.x && y == game.player.y);
        bool already_has_mob = false;
        for (int j = 0; j < game.num_mobs; ++j) {
            if (game.mobs[j].x == x && game.mobs[j].y == y) {
                already_has_mob = true;
                break;
            }
        }

        if (traversable && !occupied_by_player && !already_has_mob) {
            game.mobs[i].x = x;
            game.mobs[i].y = y;
            // Assign random health and damage to mobs
            game.mobs[i].health = rnd(50, 100); // Random health between 50 and 100
            game.mobs[i].damage = rnd(5, 15);   // Random damage between 5 and 15
            game.num_mobs++;
        }
    }
}

void draw_game_over() {
    close_window("Tile-Based RPG");
}
