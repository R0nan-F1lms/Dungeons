#include "splashkit.h"
#include <cstdlib> // Include for random number generation

/*
JSON editor
CONVERT CONSTS TO JSON
*/
//json consts_json = json_from_file("resources/json/consts.json");

using std::to_string;

// const int SCREEN_WIDTH = json_read_number(consts_json, "SCREEN_WIDTH");
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 50;
const int NUM_TILES_X = SCREEN_WIDTH / TILE_SIZE;
const int NUM_TILES_Y = SCREEN_HEIGHT / TILE_SIZE;
const int MAX_MOBS = 6;           // Maximum number of mobs on the board at all times
const int TICK_SPEED = 10;        // Every 10 ticks a new mob spawns
const int WATER_SPAWN_CHANCE = 3; // Chance for water to spawn (out of 10)
const int MAX_AIR = 100;          // Maximum air the player can have
const int MAX_HEALTH = 100;
const int AIR_GAIN_RATE = 5;
const int AIR_LOSS_RATE = 10;       // Rate of air loss per second
const int DROWN_THRESHOLD = 10;     // Time threshold (in seconds) before player starts drowning
const int MOB_MOVE_INTERVAL = 1000; // 1000 milliseconds = 1 second
const int BASE_MOBS_KILLED = 0;
const string FOOTSTEP_FIRST = "footstep1";
const string FOOTSTEP_SECOND = "footstep2";
const string WATER_SOUND_EFFECT = "water";
const string MOB_MOVE_TIMER = "mob_move_timer";
timer mob_move;
int footstepValue;




enum TileType
{
    GRASS,
    WATER,
    WALL,
    DOOR
};

enum GameState
{
    PLAYING,
    GAME_OVER,
    NOT_STARTED,
    LEVELED
};

struct Tile
{
    TileType type;
    bool traversable;
    int x, y;
};

struct Mob
{
    int x, y;
    int health;
    int damage;
    int move_direction; // 0: up, 1: down, 2: left, 3: right
};

struct Player
{
    int x, y;
    int health;
    bool has_key;
    int air;
    int mobs_killed;
    int level;
};

struct Game
{
    Player player;
    Tile world[NUM_TILES_X][NUM_TILES_Y];
    Mob mobs[MAX_MOBS];
    int num_mobs;
    GameState state;
    int tick_counter; // Counter for tick system
};

void initialize_tiles(Game &game);
void setup(Game &game);
void draw_world(Game &game);
void draw_player(const Game &game);
void draw_mobs(const Game &game);
void draw_health(const Game &game);
void draw_air(const Game &game);
void handle_input(Game &game);
bool is_traversable(Game &game, int x, int y);
void move_player(Game &game, int dx, int dy);
void update_game_state(Game &game);
void spawn_mobs(Game &game);
void move_mobs(Game &game);
void draw_game_over();
bool is_mob_at(int x, int y, const Game &game);
void leveled(Game &game);
void leveling(Game &game);
// play sounds functions
// music load_music(const string &name, const string &filename);
// void fade_music_in(music data, int times, int ms);

int main()
{
    Game game;
    open_window("Tile-Based RPG", SCREEN_WIDTH, SCREEN_HEIGHT);
    game.state = NOT_STARTED;
    create_timer(MOB_MOVE_TIMER);
    start_timer(MOB_MOVE_TIMER);
    load_music("background_music", "./SoundEffects/cinematic-time-lapse.mp3");
    load_sound_effect(FOOTSTEP_FIRST, "./SoundEffects/footstep1.ogg");
    load_sound_effect(FOOTSTEP_SECOND, "./SoundEffects/footstep2.ogg");
    do
    {
        if (game.state == NOT_STARTED)
        {
            process_events();
            setup(game);
            refresh_screen(60);
        }
        else if (game.state == PLAYING)
        {

            process_events();
            clear_screen(COLOR_WHITE);

            if (game.state == PLAYING)
            {
                handle_input(game);
                draw_world(game);
                draw_mobs(game);
                draw_player(game);
                draw_health(game);
                draw_air(game);
                update_game_state(game);
                // Increment the tick counter
                game.tick_counter++;
                // Spawn new mobs every TICK_SPEED ticks
                if (game.tick_counter >= TICK_SPEED)
                {
                    spawn_mobs(game);
                    game.tick_counter = 0; // Reset the tick counter
                }
                // Move mobs
                move_mobs(game);
            }
        refresh_screen(60);
        }
        else if (game.state == LEVELED) {
            process_events();
            leveled(game);
        }
        else if (game.state == GAME_OVER) 
        {
            process_events();
            draw_game_over();
        }
        refresh_screen(60);
    } while (!window_close_requested("Tile-Based RPG"));

    return 0;
}


void draw_screen(Game &game, const string &title, const string &welcome, const string &pressEnter)
{
    // Constants for text dimensions
    const int CHAR_WIDTH = 10;                    // Approximate width of a character in pixels
    const int LINE_HEIGHT = 20;                   // Height of a line of text in pixels
    const int MAX_LINE_WIDTH = SCREEN_WIDTH - 40; // Max line width with some padding

    // Calculate text widths manually (approximate)
    int title_width = title.length() * CHAR_WIDTH;
    int pressEnter_width = pressEnter.length() * CHAR_WIDTH;

    // Break welcome text into multiple lines if necessary
    vector<string> welcome_lines;
    int start = 0;
    while (start < welcome.length())
    {
        int end = start + MAX_LINE_WIDTH / CHAR_WIDTH;
        if (end >= welcome.length())
        {
            welcome_lines.push_back(welcome.substr(start));
            break;
        }
        int last_space = welcome.rfind(' ', end);
        if (last_space == string::npos || last_space <= start)
        {
            welcome_lines.push_back(welcome.substr(start, end - start));
            start = end;
        }
        else
        {
            welcome_lines.push_back(welcome.substr(start, last_space - start));
            start = last_space + 1;
        }
    }

    // Centering y positions
    int line1_y = SCREEN_HEIGHT / 2 - (welcome_lines.size() + 1) * LINE_HEIGHT / 2 - LINE_HEIGHT / 2;
    int line2_y = line1_y + LINE_HEIGHT;
    int line3_y = line2_y + welcome_lines.size() * LINE_HEIGHT;

    // Clear the screen
    clear_screen(COLOR_WHITE_SMOKE);

    // Draw each line of text
    draw_text(title, COLOR_BLACK, (SCREEN_WIDTH - title_width) / 2, line1_y);
    for (size_t i = 0; i < welcome_lines.size(); ++i)
    {
        int line_width = welcome_lines[i].length() * CHAR_WIDTH;
        draw_text(welcome_lines[i], COLOR_BLACK, (SCREEN_WIDTH - line_width) / 2, line2_y + i * LINE_HEIGHT);
    }
    draw_text(pressEnter, COLOR_BLACK, (SCREEN_WIDTH - pressEnter_width) / 2, line3_y);

    // Handle input to continue
    handle_input(game);
}

void initialize_tiles(Game &game)
{
    for (int i = 0; i < NUM_TILES_X; ++i)
    {
        for (int j = 0; j < NUM_TILES_Y; ++j)
        {
            game.world[i][j].x = i * TILE_SIZE;
            game.world[i][j].y = j * TILE_SIZE;
            // Set tile type and traversable attribute
            if (i == 0 || i == NUM_TILES_X - 1 || j == 0 || j == NUM_TILES_Y - 1)
            {
                game.world[i][j].type = WALL;
                game.world[i][j].traversable = false;
            }
            else if (i == NUM_TILES_X / 2 && j == NUM_TILES_Y / 2)
            {
                game.world[i][j].type = DOOR;
                game.world[i][j].traversable = false;
            }
            else
            {
                int rnd_num = rnd(10); // Generate a random number between 0 and 9
                if (rnd_num < WATER_SPAWN_CHANCE)
                {
                    game.world[i][j].type = WATER;
                    game.world[i][j].traversable = true;
                }
                else
                {
                    game.world[i][j].type = GRASS;
                    game.world[i][j].traversable = true;
                }
            }
        }
    }
}

void setup(Game &game)
{
    // Generate random spawn coordinates for the player
    int x_tile, y_tile;
    do
    {
        x_tile = rnd(NUM_TILES_X);
        y_tile = rnd(NUM_TILES_Y);
    } while (game.world[x_tile][y_tile].type == WALL);

    // Convert tile indices to screen coordinates
    game.player.x = x_tile * TILE_SIZE;
    game.player.y = y_tile * TILE_SIZE;
    game.player.health = MAX_HEALTH;
    game.player.has_key = false;
    game.player.air = MAX_AIR;
    game.player.mobs_killed = BASE_MOBS_KILLED;
    game.player.level = 1;
    game.num_mobs = 0;
    game.tick_counter = 0; // Initialize tick counter

    string title = "Tile-Based RPG";
    string welcome = "Welcome to this RPG game, developed by Ronan. To get started, please kill " +
                     to_string(game.player.level * 10 / 2) + " mobs to get the key to progress to the next level.";
    string pressEnter = "Press ENTER to Start";

    draw_screen(game, title, welcome, pressEnter);
}

void draw_world(Game &game)
{
    for (int i = 0; i < NUM_TILES_X; ++i)
    {
        for (int j = 0; j < NUM_TILES_Y; ++j)
        {
            switch (game.world[i][j].type)
            {
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
                if (game.player.has_key)
                {
                    if (game.world[i][j].type == DOOR)
                    {
                        game.world[i][j].traversable = true;
                    }
                }
                break;
            }
        }
    }
}

void draw_mobs(const Game &game)
{
    for (int i = 0; i < game.num_mobs; ++i)
    {
        fill_circle(COLOR_GRAY, game.mobs[i].x + TILE_SIZE / 2, game.mobs[i].y + TILE_SIZE / 2, TILE_SIZE / 4);
    }
}

void draw_player(const Game &game)
{
    fill_circle(COLOR_RED, game.player.x + TILE_SIZE / 2, game.player.y + TILE_SIZE / 2, TILE_SIZE / 4);
}

void draw_health(const Game &game)
{
    for (int i = 0; i < 10; ++i)
    {
        if (i < game.player.health / 10)
        {
            fill_rectangle(COLOR_RED, SCREEN_WIDTH - 100 + i * 10, 10, 10, 10);
        }
        else
        {
            fill_rectangle(COLOR_GRAY, SCREEN_WIDTH - 100 + i * 10, 10, 10, 10);
        }
    }
}

void draw_air(const Game &game)
{
    for (int i = 0; i < 10; ++i)
    {
        if (i < game.player.air / 10)
        {
            fill_rectangle(COLOR_BLUE, SCREEN_WIDTH - 100 + i * 10, 30, 10, 10);
        }
        else
        {
            fill_rectangle(COLOR_GRAY, SCREEN_WIDTH - 100 + i * 10, 30, 10, 10);
        }
    }
}

void handle_input(Game &game)
{
    if (game.state == NOT_STARTED)
    {
        if (key_typed(RETURN_KEY))
        {
            initialize_tiles(game);
            spawn_mobs(game);
            game.state = PLAYING;
        }
        else if (key_typed(ESCAPE_KEY))
        {
            draw_game_over();
        }
    }
    else if (game.state == LEVELED)
    {
        if (key_typed(RETURN_KEY))
        {
            game.state = PLAYING;
            initialize_tiles(game);
            spawn_mobs(game);
        }
        else if (key_typed(ESCAPE_KEY))
        {
            draw_game_over();
        }
    }
    else if (game.state == PLAYING)
    {
        if (key_typed(D_KEY))
        {
            move_player(game, TILE_SIZE, 0);
        }
        else if (key_typed(A_KEY))
        {
            move_player(game, -TILE_SIZE, 0);
        }
        else if (key_typed(W_KEY))
        {
            move_player(game, 0, -TILE_SIZE);
        }
        else if (key_typed(S_KEY))
        {
            move_player(game, 0, TILE_SIZE);
        }
        else if (key_typed(ESCAPE_KEY))
        {
            draw_game_over();
        }
    }
}

bool is_traversable(Game &game, int x, int y)
{
    if (x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
        return false;
    return game.world[x / TILE_SIZE][y / TILE_SIZE].traversable;
}

void leveled(Game &game)
{
    if (game.player.level <= 3)
    {
        
    string title = "LEVELED UP " + to_string(game.player.level - 1) + " -> " + to_string(game.player.level);
    string welcome = "please kill " + to_string(game.player.level * 10 / 2) + 
                    " mobs to get the key to progress to the next level.";
    string pressEnter = "Press ENTER to Continue";

    draw_screen(game, title, welcome, pressEnter);
    }
    else
    {
        game.state = GAME_OVER;
        printf("3 levels completed Game over you win!\n");
    }
}

void leveling(Game &game)
{
    // Clear the screen and reset player's position
    clear_screen(COLOR_WHITE_SMOKE);
    // Increment player's level and regenerate the world
    game.player.level++;
    game.player.has_key = false; // clearing the players key so they have to get it in the next level.
    game.player.mobs_killed = BASE_MOBS_KILLED;
    game.player.health = MAX_HEALTH;
    game.player.air = MAX_AIR;
    game.state = LEVELED;
    printf("Leveled up to level: %d\n", game.player.level);
}

void move_player(Game &game, int dx, int dy)
{
    int new_x = game.player.x + dx;
    int new_y = game.player.y + dy;

    if (is_traversable(game, new_x, new_y))
    {
        game.player.x = new_x;
        game.player.y = new_y;
        // Check for door collision
        int tile_x = game.player.x / TILE_SIZE;
        int tile_y = game.player.y / TILE_SIZE;
        if (game.world[tile_x][tile_y].type == DOOR && !game.player.has_key)
        {
            // Player needs key to open the door
            // Prevent player from moving through the door without the key
            game.player.x -= dx;
            game.player.y -= dy;
        }
        else if (game.world[tile_x][tile_y].type == WATER)
        {
            if (game.player.air < 0)
            {
                game.player.health -= game.player.level * 2;
            }
            else
            {
                game.player.air -= AIR_LOSS_RATE;
            }
        }
        else if (game.world[tile_x][tile_y].type == GRASS)
        {

            // Play footstep sound alternately
            if (footstepValue == 0)
            {
                play_sound_effect(FOOTSTEP_FIRST);
                footstepValue = 1;
                printf("footstep first played\n");
            }
            else
            {
                play_sound_effect(FOOTSTEP_SECOND);
                footstepValue = 0;
                printf("footstep second played\n");
            }
            if (game.player.air < MAX_AIR)
            {
                game.player.air += AIR_GAIN_RATE;
            }
            else
            {
                printf("player air is max %d\n", game.player.air);
            }
            int x, y;
            if (game.player.health < MAX_HEALTH)
            {
                game.player.health++;
            }
            else
            {
                printf("Player is at max health\n");
            }
            // stop_sound_effect(FOOTSTEPS);
        }
        else if (game.world[tile_x][tile_y].type == DOOR)
        {
            leveling(game);
        }
        // Check for mob collision
        for (int i = 0; i < game.num_mobs; ++i)
        {
            if (game.mobs[i].x == game.player.x && game.mobs[i].y == game.player.y)
            {
                // Decrease player's health when colliding with a mob
                game.player.health -= game.mobs[i].damage;
                game.player.mobs_killed++;
                if (game.player.mobs_killed == game.player.level * 10 / 2) // for level 1 mobs to kill is 5, for leve 2 mobs to kill is 10
                {
                    game.player.has_key = true;
                }
                // Remove the mob from the game
                for (int j = i; j < game.num_mobs - 1; ++j)
                {
                    game.mobs[j] = game.mobs[j + 1];
                }
                game.num_mobs--;
                break; // Stop checking for mob collisions once one is found
            }
        }
    }
}

void update_game_state(Game &game)
{
    // Check if player's health drops to zero
    if (game.player.health <= 0)
    {
        printf("Player died health dropped below 0\n");
        game.state = GAME_OVER;
    }
}

void spawn_mobs(Game &game)
{
    // Spawn new mobs up to MAX_MOBS
    for (int i = game.num_mobs; i < MAX_MOBS; ++i)
    {
        int x_tile = rnd(NUM_TILES_X); // Random tile x-coordinate
        int y_tile = rnd(NUM_TILES_Y); // Random tile y-coordinate

        int x = x_tile * TILE_SIZE + TILE_SIZE;
        int y = y_tile * TILE_SIZE + TILE_SIZE;

        // Check if the tile is traversable and not occupied by the player or another mob
        bool traversable = is_traversable(game, x, y);
        bool already_has_mob = is_mob_at(x, y, game);

        if (traversable && !already_has_mob)
        {
            game.mobs[i].x = x;
            game.mobs[i].y = y;
            int rnd_dmg_1 = game.player.level * 10;
            int rnd_dmg_2 = game.player.level * 15;
            // Assign random health and damage to mobs
            game.mobs[i].health = rnd(50, 100); // Random health between 50 and 100

            game.mobs[i].damage = rnd(rnd_dmg_1, rnd_dmg_2); // Random damage between 1 and 3
            game.mobs[i].move_direction = rnd(4);            // Random initial movement direction
            game.num_mobs++;
        }
    }
}

bool is_mob_at(int x, int y, const Game &game)
{
    for (int i = 0; i < game.num_mobs; ++i)
    {
        if (game.mobs[i].x == x && game.mobs[i].y == y)
        {
            return true; // Mob found at the given position
        }
    }
    return false; // No mob found at the given position
}

void move_mobs(Game &game)
{
    if (timer_ticks(MOB_MOVE_TIMER) >= MOB_MOVE_INTERVAL)
    {
        for (int i = 0; i < game.num_mobs; ++i)
        {
            // Generate random movement direction
            int move_dir = rnd(4); // 0: up, 1: down, 2: left, 3: right
            // Move mob based on direction
            switch (move_dir)
            {
            case 0:
                game.mobs[i].y -= TILE_SIZE; // Move one-half of the tile size up
                break;
            case 1:
                game.mobs[i].y += TILE_SIZE; // Move one-half of the tile size down
                break;
            case 2:
                game.mobs[i].x -= TILE_SIZE; // Move one-half of the tile size left
                break;
            case 3:
                game.mobs[i].x += TILE_SIZE; // Move one-half of the tile size right
                break;
            }
            // Ensure mob stays within bounds and moves to a traversable tile
            int new_tile_x = game.mobs[i].x / TILE_SIZE;
            int new_tile_y = game.mobs[i].y / TILE_SIZE;
            if (new_tile_x < 0 || new_tile_x >= NUM_TILES_X || new_tile_y < 0 || new_tile_y >= NUM_TILES_Y || !game.world[new_tile_x][new_tile_y].traversable)
            {
                // Undo movement if mob moves out of bounds or onto non-traversable tile
                game.mobs[i].x -= (move_dir == 3 ? TILE_SIZE : (move_dir == 2 ? -TILE_SIZE : 0)); // Undo horizontal movement
                game.mobs[i].y -= (move_dir == 1 ? TILE_SIZE : (move_dir == 0 ? -TILE_SIZE : 0)); // Undo vertical movement
            }

            // Check for mob collision
            if (game.mobs[i].x == game.player.x && game.mobs[i].y == game.player.y)
            {
                // Decrease player's health when colliding with a mob
                game.player.health -= game.mobs[i].damage;
            }
        }
        reset_timer(MOB_MOVE_TIMER);
    }
}

void draw_game_over()
{
    close_window("Tile-Based RPG");
}
