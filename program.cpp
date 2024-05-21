#include "splashkit.h"
#include <cstdlib> // Include for random number generation

/*
JSON editor
CONVERT CONSTS TO JSON : DONE
*/


using std::to_string;
timer mob_move;

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

struct Para
{
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;
    int TILE_SIZE;
    int NUM_TILES_X;
    int NUM_TILES_Y;
    int MAX_MOBS;
    int TICK_SPEED;
    int WATER_SPAWN_CHANCE;
    int MAX_AIR;
    int MAX_HEALTH;
    int AIR_GAIN_RATE;
    int AIR_LOSS_RATE;
    int DROWN_THRESHOLD;
    int MOB_MOVE_INTERVAL;
    int BASE_MOBS_KILLED;
    string FOOTSTEP_FIRST;
    string FOOTSTEP_SECOND;
    string WATER_SOUND_EFFECT;
    string MOB_MOVE_TIMER;
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
    int footstepValue;
};

struct Game
{
    Player player;
    
    Tile** world;  // Change to pointer to pointer
    Mob* mobs;
    int num_mobs;
    GameState state;
    int tick_counter; // Counter for tick system
};




void initialize_tiles(const Para &p, Game &game);
void setup(const Para &p, Game &game);
void draw_world(const Para &p, Game &game);
void draw_player(const Para &p, const Game &game);
void draw_mobs(const Para &p, const Game &game);
void draw_health(const Para &p, const Game &game);
void draw_air(const Para &p, const Game &game);
void handle_input(const Para &p,Game &game);
bool is_traversable(const Para &p, Game &game, int x, int y);
void move_player(const Para &p, Game &game, int dx, int dy);
void update_game_state(Game &game);
void spawn_mobs(const Para &p, Game &game);
void move_mobs(const Para &p,Game &game);
void draw_game_over();
bool is_mob_at(int x, int y, const Game &game);
void leveled(const Para &p,Game &game);
void leveling(const Para &p,Game &game);
void draw_mobs(const Para &p,const Game &game);
void load_constants_from_json(Para &p, const string& filename);
// play sounds functions
// music load_music(const string &name, const string &filename);
// void fade_music_in(music data, int times, int ms);

int main()
{
    Game game;
    Para p;
    load_constants_from_json(p, "consts.json");
    open_window("Tile-Based RPG", p.SCREEN_WIDTH, p.SCREEN_HEIGHT);
    game.state = NOT_STARTED;
    create_timer(p.MOB_MOVE_TIMER);
    start_timer(p.MOB_MOVE_TIMER);
    load_music("background_music", "./SoundEffects/cinematic-time-lapse.mp3");
    load_sound_effect(p.FOOTSTEP_FIRST, "./SoundEffects/footstep1.ogg");
    load_sound_effect(p.FOOTSTEP_SECOND, "./SoundEffects/footstep2.ogg");
    do
    {
        if (game.state == NOT_STARTED)
        {
            process_events();
            setup(p, game);
            refresh_screen(60);
        }
        else if (game.state == PLAYING)
        {

            process_events();
            clear_screen(COLOR_WHITE);

            if (game.state == PLAYING)
            {
                handle_input(p, game);
                draw_world(p, game);
                draw_mobs(p, game);
                draw_player(p, game);
                draw_health(p, game);
                draw_air(p, game);
                update_game_state(game);
                // Increment the tick counter
                game.tick_counter++;
                // Spawn new mobs every TICK_SPEED ticks
                if (game.tick_counter >= p.TICK_SPEED)
                {
                    spawn_mobs(p, game);
                    game.tick_counter = 0; // Reset the tick counter
                }
                // Move mobs
                move_mobs(p, game);
            }
            refresh_screen(60);
        }
        else if (game.state == LEVELED)
        {
            process_events();
            leveled(p, game);
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

void load_constants_from_json(Para &p, const string& filename)
{
    // Read JSON file
    json consts_json = json_from_file(filename);

    // Assign values to global variables
    p.SCREEN_WIDTH = json_read_number(consts_json, "SCREEN_WIDTH");
    p.SCREEN_HEIGHT = json_read_number(consts_json, "SCREEN_HEIGHT");
    p.TILE_SIZE = json_read_number(consts_json, "TILE_SIZE");
    p.NUM_TILES_X = p.SCREEN_WIDTH / p.TILE_SIZE;
    p.NUM_TILES_Y = p.SCREEN_HEIGHT / p.TILE_SIZE;
    p.MAX_MOBS = json_read_number(consts_json, "MAX_MOBS");
    p.TICK_SPEED = json_read_number(consts_json, "TICK_SPEED");\
    p.WATER_SPAWN_CHANCE = json_read_number(consts_json, "WATER_SPAWN_CHANCE");
    p.MAX_AIR = json_read_number(consts_json, "MAX_AIR");
    p.MAX_HEALTH = json_read_number(consts_json, "MAX_HEALTH");
    p.AIR_GAIN_RATE = json_read_number(consts_json, "AIR_GAIN_RATE");
    p.AIR_LOSS_RATE = json_read_number(consts_json, "AIR_LOSS_RATE");
    p.DROWN_THRESHOLD = json_read_number(consts_json, "DROWN_THRESHOLD");
    p.MOB_MOVE_INTERVAL = json_read_number(consts_json, "MOB_MOVE_INTERVAL");
    p.BASE_MOBS_KILLED = json_read_number(consts_json, "BASE_MOBS_KILLED");
    p.FOOTSTEP_FIRST = json_read_string(consts_json, "FOOTSTEP_FIRST");
    p.FOOTSTEP_SECOND = json_read_string(consts_json, "FOOTSTEP_SECOND");
    p.WATER_SOUND_EFFECT = json_read_string(consts_json, "WATER_SOUND_EFFECT");
    p.MOB_MOVE_TIMER = json_read_string(consts_json, "MOB_MOVE_TIMER");
}

void draw_screen(const Para &p,Game &game, const string &title, const string &welcome, const string &pressEnter)
{
    // Constants for text dimensions
    int CHAR_WIDTH = 10;                    // Approximate width of a character in pixels
    int LINE_HEIGHT = 20;                   // Height of a line of text in pixels
    int MAX_LINE_WIDTH = p.SCREEN_WIDTH - 40; // Max line width with some padding

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
    int line1_y = p.SCREEN_HEIGHT / 2 - (welcome_lines.size() + 1) * LINE_HEIGHT / 2 - LINE_HEIGHT / 2;
    int line2_y = line1_y + LINE_HEIGHT;
    int line3_y = line2_y + welcome_lines.size() * LINE_HEIGHT;

    // Clear the screen
    clear_screen(COLOR_WHITE_SMOKE);

    // Draw each line of text
    draw_text(title, COLOR_BLACK, (p.SCREEN_WIDTH - title_width) / 2, line1_y);
    for (size_t i = 0; i < welcome_lines.size(); ++i)
    {
        int line_width = welcome_lines[i].length() * CHAR_WIDTH;
        draw_text(welcome_lines[i], COLOR_BLACK, (p.SCREEN_WIDTH - line_width) / 2, line2_y + i * LINE_HEIGHT);
    }
    draw_text(pressEnter, COLOR_BLACK, (p.SCREEN_WIDTH - pressEnter_width) / 2, line3_y);

    // Handle input to continue
    handle_input(p, game);
}

void initialize_tiles(const Para &p, Game &game)
{
    // Allocate memory for the 2D array
    game.world = new Tile*[p.NUM_TILES_X];
    for (int i = 0; i < p.NUM_TILES_X; ++i)
    {
        game.world[i] = new Tile[p.NUM_TILES_Y];
    }

    for (int i = 0; i < p.NUM_TILES_X; ++i)
    {
        for (int j = 0; j < p.NUM_TILES_Y; ++j)
        {
            game.world[i][j].x = i * p.TILE_SIZE;
            game.world[i][j].y = j * p.TILE_SIZE;
            // Set tile type and traversable attribute
            if (i == 0 || i == p.NUM_TILES_X - 1 || j == 0 || j == p.NUM_TILES_Y - 1)
            {
                game.world[i][j].type = WALL;
                game.world[i][j].traversable = false;
            }
            else if (i == p.NUM_TILES_X / 2 && j == p.NUM_TILES_Y / 2)
            {
                game.world[i][j].type = DOOR;
                game.world[i][j].traversable = false;
            }
            else
            {
                int rnd_num = rnd(10); // Generate a random number between 0 and 9
                if (rnd_num < p.WATER_SPAWN_CHANCE)
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

void setup(const Para &p, Game &game)
{
    // Generate random spawn coordinates for the player
    int x_tile, y_tile;
    x_tile = rnd(p.NUM_TILES_X);
    y_tile = rnd(p.NUM_TILES_Y);
    // Convert tile indices to screen coordinates
    game.player.x = x_tile * p.TILE_SIZE;
    game.player.y = y_tile * p.TILE_SIZE;
    game.player.health = p.MAX_HEALTH;
    game.player.has_key = false;
    game.player.air = p.MAX_AIR;
    game.player.mobs_killed = p.BASE_MOBS_KILLED;
    game.player.level = 1;
    game.num_mobs = 0;
    game.tick_counter = 0; // Initialize tick counter

    string title = "Tile-Based RPG";
    string welcome = "Welcome to this RPG game, developed by Ronan. To get started, please kill " +
                     to_string(game.player.level * 10 / 2) + " mobs to get the key to progress to the next level.";
    string pressEnter = "Press ENTER to Start";

    draw_screen(p, game, title, welcome, pressEnter);
}

void draw_world(const Para &p,Game &game)
{
    for (int i = 0; i < p.NUM_TILES_X; ++i)
    {
        for (int j = 0; j < p.NUM_TILES_Y; ++j)
        {
            switch (game.world[i][j].type)
            {
            case GRASS:
                fill_rectangle(COLOR_GREEN, game.world[i][j].x, game.world[i][j].y, p.TILE_SIZE, p.TILE_SIZE);
                break;
            case WATER:
                fill_rectangle(COLOR_BLUE, game.world[i][j].x, game.world[i][j].y, p.TILE_SIZE, p.TILE_SIZE);
                break;
            case WALL:
                fill_rectangle(COLOR_DARK_GREEN, game.world[i][j].x, game.world[i][j].y, p.TILE_SIZE, p.TILE_SIZE);
                break;
            case DOOR:
                fill_rectangle(game.player.has_key ? COLOR_GOLD : COLOR_BLACK, game.world[i][j].x, game.world[i][j].y, p.TILE_SIZE, p.TILE_SIZE);
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

void initialize_mobs(const Para &p,Game &game)
{
    game.mobs = new Mob[p.MAX_MOBS];  // Allocate memory for the mobs array
}
void draw_mobs(const Para &p, const Game &game)
{
    for (int i = 0; i < game.num_mobs; ++i)
    {
        fill_circle(COLOR_GRAY, game.mobs[i].x + p.TILE_SIZE / 2, game.mobs[i].y + p.TILE_SIZE / 2, p.TILE_SIZE / 4);
    }
}

void draw_player(const Para &p, const Game &game)
{
    fill_circle(COLOR_RED, game.player.x + p.TILE_SIZE / 2, game.player.y + p.TILE_SIZE / 2, p.TILE_SIZE / 4);
}

void draw_health(const Para &p, const Game &game)
{
    for (int i = 0; i < 10; ++i)
    {
        if (i < game.player.health / 10)
        {
            fill_rectangle(COLOR_RED, p.SCREEN_WIDTH - 100 + i * 10, 10, 10, 10);
        }
        else
        {
            fill_rectangle(COLOR_GRAY, p.SCREEN_WIDTH - 100 + i * 10, 10, 10, 10);
        }
    }
}

void draw_air(const Para &p, const Game &game)
{
    for (int i = 0; i < 10; ++i)
    {
        if (i < game.player.air / 10)
        {
            fill_rectangle(COLOR_BLUE, p.SCREEN_WIDTH - 100 + i * 10, 30, 10, 10);
        }
        else
        {
            fill_rectangle(COLOR_GRAY, p.SCREEN_WIDTH - 100 + i * 10, 30, 10, 10);
        }
    }
}

void handle_input(const Para &p, Game &game)
{
    if (game.state == NOT_STARTED)
    {
        if (key_typed(RETURN_KEY))
        {
            initialize_tiles(p, game);
            spawn_mobs(p, game);
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
            initialize_tiles(p, game);
            spawn_mobs(p, game);
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
            move_player(p, game, p.TILE_SIZE, 0);
        }
        else if (key_typed(A_KEY))
        {
            move_player(p, game, -p.TILE_SIZE, 0);
        }
        else if (key_typed(W_KEY))
        {
            move_player(p, game, 0, -p.TILE_SIZE);
        }
        else if (key_typed(S_KEY))
        {
            move_player(p, game, 0, p.TILE_SIZE);
        }
        else if (key_typed(ESCAPE_KEY))
        {
            draw_game_over();
        }
    }
}

bool is_traversable(const Para &p,Game &game, int x, int y)
{
    if (x < 0 || y < 0 || x >= p.SCREEN_WIDTH || y >= p.SCREEN_HEIGHT)
        return false;
    return game.world[x / p.TILE_SIZE][y / p.TILE_SIZE].traversable;
}

void leveled(const Para &p,Game &game)
{
    if (game.player.level <= 3)
    {

        string title = "LEVELED UP " + to_string(game.player.level - 1) + " -> " + to_string(game.player.level);
        string welcome = "please kill " + to_string(game.player.level * 10 / 2) +
                         " mobs to get the key to progress to the next level.";
        string pressEnter = "Press ENTER to Continue";

        draw_screen(p, game, title, welcome, pressEnter);
    }
    else
    {
        game.state = GAME_OVER;
        printf("3 levels completed Game over you win!\n");
    }
}

void leveling(const Para &p, Game &game)
{
    // Clear the screen and reset player's position
    clear_screen(COLOR_WHITE_SMOKE);
    // Increment player's level and regenerate the world
    game.player.level++;
    game.player.has_key = false; // clearing the players key so they have to get it in the next level.
    game.player.mobs_killed = p.BASE_MOBS_KILLED;
    game.player.health = p.MAX_HEALTH;
    game.player.air = p.MAX_AIR;
    game.state = LEVELED;
    printf("Leveled up to level: %d\n", game.player.level);
}

void move_player(const Para &p, Game &game, int dx, int dy)
{
    int new_x = game.player.x + dx;
    int new_y = game.player.y + dy;

    if (is_traversable(p, game, new_x, new_y))
    {
        game.player.x = new_x;
        game.player.y = new_y;
        // Check for door collision
        int tile_x = game.player.x / p.TILE_SIZE;
        int tile_y = game.player.y / p.TILE_SIZE;
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
                game.player.air -= p.AIR_LOSS_RATE;
            }
        }
        else if (game.world[tile_x][tile_y].type == GRASS)
        {

            // Play footstep sound alternately
            if (game.player.footstepValue == 0)
            {
                play_sound_effect(p.FOOTSTEP_FIRST);
                game.player.footstepValue = 1;
                printf("footstep first played\n");
            }
            else
            {
                play_sound_effect(p.FOOTSTEP_SECOND);
                game.player.footstepValue = 0;
                printf("footstep second played\n");
            }
            if (game.player.air < p.MAX_AIR)
            {
                game.player.air += p.AIR_GAIN_RATE;
            }
            else
            {
                printf("player air is max %d\n", game.player.air);
            }
            int x, y;
            if (game.player.health < p.MAX_HEALTH)
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
            leveling(p, game);
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

void spawn_mobs(const Para &p, Game &game)
{
    // Spawn new mobs up to MAX_MOBS
    for (int i = game.num_mobs; i < p.MAX_MOBS; ++i)
    {
        int x_tile = rnd(p.NUM_TILES_X); // Random tile x-coordinate
        int y_tile = rnd(p.NUM_TILES_Y); // Random tile y-coordinate

        int x = x_tile * p.TILE_SIZE + p.TILE_SIZE;
        int y = y_tile * p.TILE_SIZE + p.TILE_SIZE;

        // Check if the tile is traversable and not occupied by the player or another mob
        bool traversable = is_traversable(p, game, x, y);
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

void move_mobs(const Para &p, Game &game)
{
    if (timer_ticks(p.MOB_MOVE_TIMER) >= p.MOB_MOVE_INTERVAL)
    {
        for (int i = 0; i < game.num_mobs; ++i)
        {
            // Generate random movement direction
            int move_dir = rnd(4); // 0: up, 1: down, 2: left, 3: right
            // Move mob based on direction
            switch (move_dir)
            {
            case 0:
                game.mobs[i].y -= p.TILE_SIZE; // Move one-half of the tile size up
                break;
            case 1:
                game.mobs[i].y += p.TILE_SIZE; // Move one-half of the tile size down
                break;
            case 2:
                game.mobs[i].x -= p.TILE_SIZE; // Move one-half of the tile size left
                break;
            case 3:
                game.mobs[i].x += p.TILE_SIZE; // Move one-half of the tile size right
                break;
            }
            // Ensure mob stays within bounds and moves to a traversable tile
            int new_tile_x = game.mobs[i].x / p.TILE_SIZE;
            int new_tile_y = game.mobs[i].y / p.TILE_SIZE;
            if (new_tile_x < 0 || new_tile_x >= p.NUM_TILES_X || new_tile_y < 0 || new_tile_y >= p.NUM_TILES_Y || !game.world[new_tile_x][new_tile_y].traversable)
            {
                // Undo movement if mob moves out of bounds or onto non-traversable tile
                game.mobs[i].x -= (move_dir == 3 ? p.TILE_SIZE : (move_dir == 2 ? -p.TILE_SIZE : 0)); // Undo horizontal movement
                game.mobs[i].y -= (move_dir == 1 ? p.TILE_SIZE : (move_dir == 0 ? -p.TILE_SIZE : 0)); // Undo vertical movement
            }

            // Check for mob collision
            if (game.mobs[i].x == game.player.x && game.mobs[i].y == game.player.y)
            {
                // Decrease player's health when colliding with a mob
                game.player.health -= game.mobs[i].damage;
            }
        }
        reset_timer(p.MOB_MOVE_TIMER);
    }
}

void draw_game_over()
{
    close_window("Tile-Based RPG");
}
