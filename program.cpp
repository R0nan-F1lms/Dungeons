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
    LEVELED,
    EDITING
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
    Tile **world; // Change to pointer to pointer
    Mob *mobs;
    int num_mobs;
    GameState state;
    int tick_counter; // Counter for tick system
    string map;
    
};

void initialize_tiles(const string &filename,const Para &p, Game &game);
void setup(const Para &p, Game &game);
void draw_world(const Para &p, Game &game);
void draw_player(const Para &p, const Game &game);
void draw_mobs(const Para &p, const Game &game);
void draw_stats(const Para &p, const Game &game);
void handle_input(const Para &p, Game &game);
bool is_traversable(const Para &p, Game &game, int x, int y);
void move_player(const Para &p, Game &game, int dx, int dy);
void update_game_state(Game &game);
void spawn_mobs(const Para &p, Game &game);
void move_mobs(const Para &p, Game &game);
void draw_game_over();
bool is_mob_at(int x, int y, const Game &game);
void leveled(const Para &p, Game &game);
void leveling(const Para &p, Game &game);
void load_constants_from_json(Para &p, const string &filename);
void edit_map(const Para &p, Game &game, TileType draw_type);
void save_map_to_file(const std::string &filename, const Para &p, const Game &game);
string tile_type_to_string(TileType type);
void display_commands();

// Function to convert TileType to string
string tile_type_to_string(TileType type) {
    switch (type) {
        case GRASS: return "GRASS";
        case WATER: return "WATER";
        case WALL: return "WALL";
        case DOOR: return "DOOR";
        default: return "UNKNOWN";
    }
}

void save_map_to_file(const std::string &filename, const Para &p, const Game &game) {
    // Create a JSON object to hold the map data
    json map_json = create_json();

    // Create a vector to hold the rows of tiles as JSON objects
    vector<json> tile_rows;

    // Iterate through the tiles and store their data in the vector
    for (int i = 0; i < p.NUM_TILES_X; ++i)
    {
        // Create a vector to represent the current row
        vector<json> tile_row;

        for (int j = 0; j < p.NUM_TILES_Y; ++j)
        {
            // Create a JSON object for the current tile
            json tile_json = create_json();

            // Store the tile type and traversable status
            json_set_number(tile_json, "type", game.world[i][j].type);
            json_set_bool(tile_json, "traversable", game.world[i][j].traversable);

            // Add the tile JSON object to the current row vector
            tile_row.push_back(tile_json);
        }

        // Add the current row vector to the rows vector as a JSON array
        json row_json = create_json();
        json_set_array(row_json, "row", tile_row);
        tile_rows.push_back(row_json);
    }

    // Add the rows vector to the map JSON object as a JSON array
    json_set_array(map_json, "tiles", tile_rows);

    // Save the JSON object to a file
    json_to_file(map_json, filename.c_str());
}

void load_constants_from_json(Para &p, const string &filename)
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
    p.TICK_SPEED = json_read_number(consts_json, "TICK_SPEED");
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

void draw_screen(const Para &p, Game &game, const string &title, const string &welcome, const string &pressEnter)
{
    // Constants for text dimensions
    int CHAR_WIDTH = 10;                      // Approximate width of a character in pixels
    int LINE_HEIGHT = 20;                     // Height of a line of text in pixels
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

// Function to load map from JSON
bool load_map_from_json(const string &filename, const Para &p, Game &game)
{
    printf("Load map from json\n");
    // Load JSON from file
    json map_json = json_from_file(filename);

    // Check if the JSON has the "tiles" key
    if (!json_has_key(map_json, "tiles"))
    {
        return false;
    }

    // Read the tiles array
    vector<json> tile_rows;
    json_read_array(map_json, "tiles", tile_rows);

    // Allocate memory for the 2D array
    game.world = new Tile *[p.NUM_TILES_X];
    for (int i = 0; i < p.NUM_TILES_X; ++i)
    {
        game.world[i] = new Tile[p.NUM_TILES_Y];
    }

    // Iterate through the rows and columns
    for (int i = 0; i < p.NUM_TILES_X; ++i)
    {
        vector<json> tile_row;
        json_read_array(tile_rows[i], "row", tile_row);

        for (int j = 0; j < p.NUM_TILES_Y; ++j)
        {
            json tile_json = tile_row[j];
            game.world[i][j].x = i * p.TILE_SIZE;
            game.world[i][j].y = j * p.TILE_SIZE;
            game.world[i][j].type = static_cast<TileType>(json_read_number_as_int(tile_json, "type"));
            game.world[i][j].traversable = json_read_bool(tile_json, "traversable");
        }
    }

    // Free the JSON object
    free_json(map_json);

    return true;
}

void initialize_tiles(const string &filename, const Para &p, Game &game)
{
    // Try to load the map from the JSON file
    if (load_map_from_json(filename, p, game))
    {
        // Successfully loaded map from file
        return;
    }

    // // If loading the map failed, generate a new map
    // // Allocate memory for the 2D array
    // game.world = new Tile*[p.NUM_TILES_X];
    // for (int i = 0; i < p.NUM_TILES_X; ++i)
    // {
    //     game.world[i] = new Tile[p.NUM_TILES_Y];
    // }

    // for (int i = 0; i < p.NUM_TILES_X; ++i)
    // {
    //     for (int j = 0; j < p.NUM_TILES_Y; ++j)
    //     {
    //         game.world[i][j].x = i * p.TILE_SIZE;
    //         game.world[i][j].y = j * p.TILE_SIZE;
    //         // Set tile type and traversable attribute
    //         if (i == 0 || i == p.NUM_TILES_X - 1 || j == 0 || j == p.NUM_TILES_Y - 1)
    //         {
    //             game.world[i][j].type = WALL;
    //             game.world[i][j].traversable = false;
    //         }
    //         else if (i == p.NUM_TILES_X / 2 && j == p.NUM_TILES_Y / 2)
    //         {
    //             game.world[i][j].type = DOOR;
    //             game.world[i][j].traversable = false;
    //         }
    //         else
    //         {
    //             int rnd_num = rnd(10); // Generate a random number between 0 and 9
    //             if (rnd_num < p.WATER_SPAWN_CHANCE)
    //             {
    //                 game.world[i][j].type = WATER;
    //                 game.world[i][j].traversable = true;
    //             }
    //             else
    //             {
    //                 game.world[i][j].type = GRASS;
    //                 game.world[i][j].traversable = true;
    //             }
    //         }
    //     }
    // }
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
    game.map = "level_"+to_string(game.player.level)+".json";
    game.mobs = new Mob[p.MAX_MOBS];

    string title = "Tile-Based RPG";
    string welcome = "Welcome to this RPG game, developed by Ronan. To get started, please kill " +
                     to_string(game.player.level * 10 / 2) + " mobs to get the key to progress to the next level.";
    string pressEnter = "Press ENTER to Start";

    draw_screen(p, game, title, welcome, pressEnter);
}

void draw_world(const Para &p, Game &game)
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

void initialize_mobs(const Para &p, Game &game)
{
    game.mobs = new Mob[p.MAX_MOBS]; // Allocate memory for the mobs array
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

void draw_stats(const Para &p, const Game &game)
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

void edit_map(const Para &p, Game &game, TileType draw_type)
{
    // Mouse position
    int m_x = mouse_x();
    int m_y = mouse_y();

    // Convert mouse position to grid coordinates
    int tile_x = m_x / p.TILE_SIZE;
    int tile_y = m_y / p.TILE_SIZE;
    // Check if the mouse is within the map bounds and left mouse button is clicked
    if (mouse_clicked(LEFT_BUTTON) && tile_x >= 0 && tile_x < p.NUM_TILES_X && tile_y >= 0 && tile_y < p.NUM_TILES_Y)
    {
        if (draw_type == GRASS)
        {
            game.world[tile_x][tile_y].type = GRASS;
            game.world[tile_x][tile_y].traversable = true;
        }
        else if (draw_type == WATER)
        {
            game.world[tile_x][tile_y].type = WATER;
            game.world[tile_x][tile_y].traversable = true;
        }
        else if (draw_type == WALL)
        {
            if (tile_x != 0 && tile_x != p.NUM_TILES_X - 1 && tile_y != 0 && tile_y != p.NUM_TILES_Y - 1)
            {
                game.world[tile_x][tile_y].type = WALL;
                game.world[tile_x][tile_y].traversable = false;
            }
        }
        else if (draw_type == DOOR)
        {
            bool door_exists = false;
            for (int i = 0; i < p.NUM_TILES_X; ++i)
            {
                for (int j = 0; j < p.NUM_TILES_Y; ++j)
                {
                    if (game.world[i][j].type == DOOR)
                    {
                        door_exists = true;
                        break;
                    }
                }
                if (door_exists)
                {
                    break;
                }
            }
            if (!door_exists)
            {
                game.world[tile_x][tile_y].type = DOOR;
                game.world[tile_x][tile_y].traversable = false;
            }
        }
    }
}

void handle_input(const Para &p, Game &game)
{
    static TileType current_draw_type = GRASS;

    if (game.state == NOT_STARTED)
    {
        if (key_typed(RETURN_KEY))
        {
            initialize_tiles(game.map, p, game);
            spawn_mobs(p, game);
            game.state = PLAYING;
        }
        else if (key_typed(ESCAPE_KEY))
        {
            game.state = GAME_OVER;
            draw_game_over();
        }
        else if (key_typed(NUM_0_KEY))
        {
            printf("Entered Map Edit Mode\n\nEditing the map");
            initialize_tiles(game.map, p, game);
            game.state = EDITING;
        }
    }
    else if (game.state == EDITING)
    {
        if (key_typed(NUM_7_KEY))
        {
            current_draw_type = GRASS;
        }
        else if (key_typed(NUM_8_KEY))
        {
            current_draw_type = WATER;
        }
        else if (key_typed(NUM_9_KEY))
        {
            current_draw_type = WALL;
        }
        else if (key_typed(NUM_6_KEY))
        {
            current_draw_type = DOOR;
        }
        else if (key_typed(NUM_1_KEY))
        {
            printf("Level 1 Map opened");
            game.map = "level_1.json";
            initialize_tiles(game.map, p, game);
        }
        else if (key_typed(NUM_2_KEY))
        {
            printf("Level 2 Map opened");
            game.map = "level_2.json";
            initialize_tiles(game.map, p, game);
        }
        else if (key_typed(NUM_3_KEY))
        {
            printf("Level 3 Map opened");
            game.map = "level_3.json";
            initialize_tiles(game.map, p, game);
        }
        else if (key_typed(ESCAPE_KEY))
        {
            game.state = NOT_STARTED;
        }
        else if (key_typed(RETURN_KEY)) {
            save_map_to_file(game.map, p, game);
            game.state = NOT_STARTED;  // Return to the initial state after saving
        }
        
        // Call edit_map to handle the drawing
        edit_map(p, game, current_draw_type);
    }
    else if (game.state==GAME_OVER){
        if (key_typed(RETURN_KEY)){
            game.state = NOT_STARTED;
        } 
        else if (key_typed(ESCAPE_KEY)){
            draw_game_over();
        }
    }
    else if (game.state == LEVELED)
    {
        if (key_typed(RETURN_KEY))
        {
            game.state = PLAYING;
            initialize_tiles(game.map, p, game);
            spawn_mobs(p, game);
        }
        else if (key_typed(ESCAPE_KEY))
        {
            game.state = GAME_OVER;
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
            game.state = GAME_OVER;
            draw_game_over();
        }
    }
}

bool is_traversable(const Para &p, Game &game, int x, int y)
{
    if (x < 0 || y < 0 || x >= p.SCREEN_WIDTH || y >= p.SCREEN_HEIGHT)
        return false;
    return game.world[x / p.TILE_SIZE][y / p.TILE_SIZE].traversable;
}

void leveled(const Para &p, Game &game)
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
    game.map = "level_"+to_string(game.player.level)+".json";
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

void spawn_mobs(const Para &p, Game &game) {
    // Check if the maximum number of mobs has been reached
    if (game.num_mobs >= p.MAX_MOBS) {
        return;
    }

    // Calculate number of mobs to spawn
    int mobs_to_spawn = p.MAX_MOBS - game.num_mobs;

    for (int i = 0; i < mobs_to_spawn; ++i) {
        int x_tile, y_tile;
        
        // Ensure mobs spawn in traversable tiles
        do {
            x_tile = rnd(p.NUM_TILES_X);
            y_tile = rnd(p.NUM_TILES_Y);
        } while (!game.world[x_tile][y_tile].traversable || is_mob_at(x_tile * p.TILE_SIZE, y_tile * p.TILE_SIZE, game));

        // Initialize the mob
        game.mobs[game.num_mobs].x = x_tile * p.TILE_SIZE;
        game.mobs[game.num_mobs].y = y_tile * p.TILE_SIZE;
        game.mobs[game.num_mobs].health = 100; // Example health value
        game.mobs[game.num_mobs].damage = 10; // Example damage value
        game.mobs[game.num_mobs].move_direction = rnd(4); // Random initial direction

        // Increment the mob count
        game.num_mobs++;
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

// Function to display the available commands
void display_commands() {
    // Define the text to be displayed
    string commands_text = "1: Level 1, 2: Level 2, 3: Level 3, 6: Door, 7: Grass 8: Water 9: Wall, Press Enter to save";
// Calculate the position to draw the text
    float x = 25; // Left side of the screen
    float y = screen_height() - 25; // 100 pixels above the bottom border
    // Draw the text on the screen
    draw_text(commands_text, COLOR_BLACK, x, y);
}

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
                draw_stats(p, game);
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
            string title = "Tile-Based RPG";
            string welcome = "Congratulations you completed the game. You can now play again or finish (escape)";
            string pressEnter = "Press ENTER to restart";

    draw_screen(p, game, title, welcome, pressEnter);
        }
        else if (game.state == EDITING) {
            process_events();
            
            handle_input(p, game);
            draw_world(p,game);
            display_commands();
        }
        else 
        {
            game.state = GAME_OVER;
        }
        refresh_screen(60);
    } while (!window_close_requested("Tile-Based RPG"));

    return 0;
}
