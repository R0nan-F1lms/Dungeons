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
            } else if (i == NUM_TILES_X / 2 && j == NUM_TILES_Y / 2) {
                game.world[i][j].type = DOOR;
                game.world[i][j].traversable = false;
            } else {
                int rnd_num = rnd(10); // Generate a random number between 0 and 9
                if (rnd_num < WATER_SPAWN_CHANCE) {
                    game.world[i][j].type = WATER;
                    game.world[i][j].traversable = false;
                } else {
                    game.world[i][j].type = GRASS;
                    game.world[i][j].traversable = true;
                }
            }
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
                case WALL:
                    fill_rectangle(COLOR_DARK_GREEN, game.world[i][j].x, game.world[i][j].y, TILE_SIZE, TILE_SIZE);
                    break;
                case DOOR:
                    fill_rectangle(game.player.has_key ? COLOR_GOLD : COLOR_BLACK, game.world[i][j].x, game.world[i][j].y, TILE_SIZE, TILE_SIZE);
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

void draw_health(const Game &game) {
    for (int i = 0; i < 10; ++i) {
        if (i < game.player.health) {
            fill_rectangle(COLOR_RED, SCREEN_WIDTH - 100 + i * 10, 10, 10, 10);
        } else {
            fill_rectangle(COLOR_GRAY, SCREEN_WIDTH - 100 + i * 10, 10, 10, 10);
        }
    }
}

void draw_air(const Game &game) {
    for (int i = 0; i < 10; ++i) {
        if (i < game.player.air / 10) {
            fill_rectangle(COLOR_BLUE, SCREEN_WIDTH - 100 + i * 10, 30, 10, 10);
        } else {
            fill_rectangle(COLOR_GRAY, SCREEN_WIDTH - 100 + i * 10, 30, 10, 10);
        }
    }
}

void draw_score(const Game &game) {
    // Draw the player's key status on the screen
    //draw_text("Has Key: " + (game.player.has_key ? "Yes" : "No"), COLOR_BLACK, 10, 50);
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
        // Check for door collision
        int tile_x = game.player.x / TILE_SIZE;
        int tile_y = game.player.y / TILE_SIZE;
        if (game.world[tile_x][tile_y].type == DOOR && !game.player.has_key) {
            // Player needs key to open the door
            // Prevent player from moving through the door without the key
            game.player.x -= dx;
            game.player.y -= dy;
        }
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
    if (game.player.health <= 0 || game.player.air <= 0) {
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
            game.mobs[i].move_direction = rnd(4); // Random initial movement direction
            game.num_mobs++;
        }
    }
}

void move_mobs(Game &game) {
    for (int i = 0; i < game.num_mobs; ++i) {
        // Generate random movement direction
        int move_dir = rnd(4); // 0: up, 1: down, 2: left, 3: right
        // Move mob based on direction
        switch (move_dir) {
            case 0:
                game.mobs[i].y -= TILE_SIZE / 2; // Move slower in water
                break;
            case 1:
                game.mobs[i].y += TILE_SIZE / 2;
                break;
            case 2:
                game.mobs[i].x -= TILE_SIZE / 2;
                break;
            case 3:
                game.mobs[i].x += TILE_SIZE / 2;
                break;
        }
        // Ensure mob stays within bounds and moves to a traversable tile
        int new_tile_x = game.mobs[i].x / TILE_SIZE;
        int new_tile_y = game.mobs[i].y / TILE_SIZE;
        if (new_tile_x < 0 || new_tile_x >= NUM_TILES_X || new_tile_y < 0 || new_tile_y >= NUM_TILES_Y || !game.world[new_tile_x][new_tile_y].traversable) {
            // Undo movement if mob moves out of bounds or onto non-traversable tile
            game.mobs[i].x = game.mobs[i].x - (move_dir == 3 ? TILE_SIZE / 2 : (move_dir == 2 ? -TILE_SIZE / 2 : 0)); // Undo horizontal movement
            game.mobs[i].y = game.mobs[i].y - (move_dir == 1 ? TILE_SIZE / 2 : (move_dir == 0 ? -TILE_SIZE / 2 : 0)); // Undo vertical movement
        }
    }
}

void draw_game_over() {
    close_window("Tile-Based RPG");
}
