#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CLEAR_TERM()   printf("\033[2J")
#define RESET_CURSOR() printf("\033[H")

#define MIN_SURVIVAL 1
#define MAX_SURVIVAL 4
#define SPAWN        3
#define INTERVAL     100

typedef struct
{
    int alive;
    size_t x;
    size_t y;
} cell;

typedef struct
{
    cell **cells;
    size_t width;
    size_t height;
    int iteration;
} world;

static world *world_init(int width, int height);
static world *world_copy(const world *w);
static void   world_draw(const world *w);
static void   world_randomize(world *w);
static void   world_mainloop(world *w);
static void   world_free(world *w);
static int    world_advance(world *w);
static int    world_compare(const world *w1, const world *w2);
static int    world_is_empty(const world *w);
static int    world_is_in_bounds(const world *w, size_t x, size_t y);
static int    cell_update(const world *w, cell *c);

/*
 * Constructs a `world` struct, and initialises it with sensible default
 * values. Returns a pointer to the heap allocated structure that ought to be
 * freed before the program terminates (ideally using `world_free()`).
 */
static world *
world_init(int width, int height)
{
    /* Initialise basic information about the simulation. */
    world *w     = malloc(sizeof *w);
    w->cells     = calloc(height, sizeof *w->cells);
    w->width     = width;
    w->height    = height;
    w->iteration = 0;

    /* Initialise each cell in the world. */
    for (size_t y = 0; y < height; y++)
    {
        w->cells[y] = calloc(width, sizeof *w->cells[y]);

        for (size_t x = 0; x < width; x++)
        {
            cell new_cell = {
                .alive = 0,
                .x     = x,
                .y     = y,
            };

            w->cells[y][x] = new_cell;
        }
    }

    return w;
}

/*
 * Creates a deep copy of a `world` structure. This is preferable to using
 * `memcpy()` as it allocates new memory for the copy instead of borrowing
 * pointers from the original struct. Returns a pointer to the heap allocated
 * copy that ought to be freed before the program terminates (ideally using
 * `world_free()`).
 */
static world *
world_copy(const world *w)
{
    /* Allocate memory for our new `world` struct. */
    world *copy = world_init(w->width, w->height);

    /* Copy the living state of each cell. */
    for (size_t y = 0; y < w->height; y++)
        for (size_t x = 0; x < w->width; x++)
            copy->cells[y][x].alive = w->cells[y][x].alive;

    return copy;
}

/*
 * Prints the contents of a `world` struct to stdout.
 */
static void
world_draw(const world *w)
{
    /* Return the cursor to (0, 0) to 'redraw' over the previous iteration. */
    RESET_CURSOR();

    /* Draw the living state for each cell in the world. */
    for (size_t y = 0; y < w->height; y++)
    {
        for (size_t x = 0; x < w->width; x++)
            printf("%c", w->cells[y][x].alive ? 'o' : ' ');

        printf("\n");
    }

    /* Draw information about the simulation. */
    printf("\vwidth: %ld, height: %ld, iteration: %d\n",
           w->width,
           w->height,
           w->iteration);
}

/*
 * Randomises the living state for each cell in a `world` struct.
 */
static void
world_randomize(world *w)
{
    /* This could cause some trouble if `world_randomize()` were to be called
       in a loop, but this function is not expected to be used in such a
       way. */
    srand(time(NULL));

    /* Randomise the living state for each cell. */
    for (size_t y = 0; y < w->height; y++)
        for (size_t x = 0; x < w->width; x++)
            w->cells[y][x].alive = rand() % 2;
}

/*
 * Runs a simulation.
 */
static void
world_mainloop(world *w)
{
    do
    {
        world_draw(w);
        usleep(INTERVAL * 1000);
    } while (world_advance(w));
}

/*
 * Frees the heap memory allocated for an initialised `world` struct.
 */
static void
world_free(world *w)
{
    for (size_t y = 0; y < w->height; y++)
        free(w->cells[y]);

    free(w->cells);
    free(w);
}

/*
 * Iterate the simulation by 1 frame. This means updating the living state of
 * each cell in the provided `world` struct. Returns 1 if the simulation is
 * still running and 0 otherwise.
 */
static int
world_advance(world *w)
{
    world *prev = world_copy(w);

    /* Update each cell in the world. */
    for (size_t y = 0; y < w->height; y++)
        for (size_t x = 0; x < w->width; x++)
            cell_update(w, &w->cells[y][x]);

    /* Compare the current iteration to the previous. If nothing has changed,
       the simulation is over. */
    int is_same_as_prev = !world_compare(w, prev);
    world_free(prev);

    /* Increment the iteration counter so long as the simulation hasn't
       ended. */
    w->iteration += !is_same_as_prev;

    /* Returns 1 if the simulation is still running and 0 otherwise. */
    return !(is_same_as_prev || world_is_empty(w));
}

/*
 * Compares the values stored by two `world` structs (does not compare
 * iteration count). Returns 0 if the worlds contain identical data, and 1
 * otherwise.
 */
static int
world_compare(const world *w1, const world *w2)
{
    /* Compare the worlds' dimensions. */
    if (w1->width != w2->width || w1->height != w2->height)
        return 1;

    /* Compare the living states of both worlds' cells. */
    for (size_t y = 0; y < w1->height; y++)
        for (size_t x = 0; x < w1->width; x++)
            if (w1->cells[y][x].alive != w2->cells[y][x].alive)
                return 1;

    return 0;
}

/*
 * Calculates whether a world has no living cells left. Returns 1 if the world
 * contains no living cells, and 0 otherwise.
 */
static int
world_is_empty(const world *w)
{
    for (size_t y = 0; y < w->height; y++)
        for (size_t x = 0; x < w->width; x++)
            if (w->cells[y][x].alive)
                return 0;

    return 1;
}

/*
 * Calculates whether a coordinate value is within a world's bounds. Returns 1
 * if both `x` and `y` are in bounds, and 0 otherwise.
 */
static int
world_is_in_bounds(const world *w, size_t x, size_t y)
{
    size_t x_in_bounds = 0 <= x && x < w->width;
    size_t y_in_bounds = 0 <= y && y < w->height;

    return x_in_bounds && y_in_bounds;
}

/*
 * Calculates the next living condition for a cell in the simulation using the
 * rules defined by the `MIN_SURVIVAL`, `MAX_SURVIVAL` and `SPAWN` macros.
 * Returns 1 if the cell is alive, and 0 otherwise.
 */
static int
cell_update(const world *w, cell *c)
{
    /* Count the cell's living neighbours. */
    int alive_neighbours = 0;
    for (int y_offset = -1; y_offset <= 1; y_offset++)
    {
        for (int x_offset = -1; x_offset <= 1; x_offset++)
        {
            int neighbour_x = c->x + x_offset;
            int neighbour_y = c->y + y_offset;

            /* Count every neighbouring cell with an `alive` value of 1. */
            int is_neighbour_current_cell = x_offset == 0 && y_offset == 0;
            if (!is_neighbour_current_cell && world_is_in_bounds(w, neighbour_x, neighbour_y))
                alive_neighbours += w->cells[neighbour_y][neighbour_x].alive;
        }
    }

    /* Calculate the cell's next living state using the simulation's rules. */
    c->alive = (c->alive)
                   ? MIN_SURVIVAL < alive_neighbours && alive_neighbours < MAX_SURVIVAL
                   : alive_neighbours == SPAWN;

    return c->alive;
}

/*
 * This program is designed to run Conway's game of life. It accepts the width
 * and height for the simulation as arguments, and randomises the living state
 * for each cell during initialisation.
 */
int
main(int argc, char **argv)
{
    /* Validate that the correct number of arguments were provided. */
    const int EXPECTED_ARGC = 3;
    if (argc != EXPECTED_ARGC)
    {
        puts("Usage: game-of-life <width> <height>");
        return EXIT_FAILURE;
    }

    /* Clear the terminal to improve formatting. */
    CLEAR_TERM();

    /* Parse the width and height arguments. */
    size_t width  = atoi(argv[1]);
    size_t height = atoi(argv[2]);

    /* Run the simulation. */
    world *w = world_init(width, height);
    world_randomize(w);
    world_mainloop(w);
    world_free(w);

    return EXIT_SUCCESS;
}
