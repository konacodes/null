# null Language Projects

Fun example projects demonstrating the null language libraries.

## Running Projects

```bash
# JIT compile and run
./null projects/dice_game.null

# Or use the interpreter
./null interp projects/fizzbuzz.null
```

## Available Projects

| Project | Description |
|---------|-------------|
| `dice_game.null` | Colorful dice roller with D&D-style rolls |
| `fizzbuzz.null` | Classic FizzBuzz with colorful output |
| `primes.null` | Prime number finder with formatting |
| `progress_demo.null` | Progress bar and number formatting demo |
| `multiplication_table.null` | Colorful 10x10 multiplication table |

## Project Descriptions

### dice_game.null
Roll multiple dice with colorful output. Features:
- Roll 3d6 (D&D ability scores)
- D20 rolls with critical hit/fail detection
- Color-coded results (green for max, red for min)

### fizzbuzz.null
The classic FizzBuzz problem with colored output:
- Numbers divisible by 3: Green "Fizz"
- Numbers divisible by 5: Blue "Buzz"
- Divisible by both: Magenta "FizzBuzz"

### primes.null
Find and display prime numbers:
- Lists all primes from 1-100
- Tests larger prime numbers
- Shows count of primes found

### progress_demo.null
Demonstrates formatting capabilities:
- Progress bars at different completion levels
- Right-aligned numbers
- Zero-padded numbers
- Color-coded progress states

### multiplication_table.null
A colorful 10x10 multiplication table:
- Color-coded by value ranges
- Clean grid formatting
- Header row and column

## Creating Your Own Projects

1. Create a new `.null` file in this directory
2. Import the libraries you need:
   ```null
   @use "std/io.null"
   @use "libs/color/color.null"
   @use "libs/random/random.null"
   ```
3. Write your main function
4. Run with `./null projects/your_project.null`
