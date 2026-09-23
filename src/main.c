#include "stm32f4xx.h"

#define LED_PORT GPIOD
#define SWITCH_PORT GPIOC
#define SW1_PIN 8

#define ANALOG_PIN 2
#define ANALOG_PORT GPIOC
#define ADC_CHANNEL 12 // ADC Channel for PC2
#define ADC_SAMPLES 16 // Number of samples for averaging

// buttons
#define BUTTON_PE6_PIN 6U
#define BUTTON_PF8_PIN 8U
#define BUTTON_PF9_PIN 9U
#define BUTTON_PE6_MASK (1U << BUTTON_PE6_PIN)
#define BUTTON_PF8_MASK (1U << BUTTON_PF8_PIN)
#define BUTTON_PF9_MASK (1U << BUTTON_PF9_PIN)
#define ALL_BUTTONS_MASK (BUTTON_PE6_MASK | \
                          BUTTON_PF8_MASK | \
                          BUTTON_PF9_MASK)

uint16_t speed = 0;    // initialize global speed variable
uint8_t direction = 0; // 0 = paused, 1 = shift left, 2 = shift right

void init_buttons(void);
uint8_t read_switches(void);
static uint8_t rotate_4bit_window(uint8_t pattern, uint8_t start_bit);

void delay(volatile uint32_t count)
{
    while (count--)
        ;
}

static uint8_t rotate_4bit_window(uint8_t pattern, uint8_t start_bit)
{
    uint8_t out = 0U;

    for (uint8_t i = 0U; i < 4U; ++i)
    {
        if ((pattern & (1U << i)) != 0U)
        {
            out |= (1U << ((start_bit + i) & 7U));
        }
    }

    return out;
}

int main(void)
{
    // enable clock
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIODEN);
    /*
    Mode register: 00 = input, 01 = output, 10 = alternate function, 11 = analog
    */
    LED_PORT->MODER &= ~(0xFFFF);
    LED_PORT->MODER |= (0x5555);

    SWITCH_PORT->MODER &= ~(0xFFu << (8 * 2));
    SWITCH_PORT->PUPDR &= ~(3U << (8 * 2U));

    LED_PORT->ODR &= ~(0xFF);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    ANALOG_PORT->MODER &= ~(0x3 << (ANALOG_PIN * 2));
    ANALOG_PORT->MODER |= (0x3 << (ANALOG_PIN * 2));

    // Initialize ADC, Default resolution is 12 bits
    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;                  // Enable ADC3 clock
    ADC3->SQR3 = ADC_CHANNEL;                            // Select channel
    ADC3->SMPR1 = ADC_SMPR1_SMP12_0 | ADC_SMPR1_SMP12_1; // Sample time 56 cycles (0b011)
    ADC3->CR2 = ADC_CR2_ADON;                            // Enable ADC

    init_buttons();

    // configure LEDs
    uint8_t leds = 0;

    // initialize timer
    uint8_t t = 0;
    static uint8_t shift = 0;

    while (1)
    {
        uint8_t sw = read_switches();

        if (direction == 1)
        {
            shift = (shift + 1U) & 7U;
            LED_PORT->ODR = rotate_4bit_window(sw, shift);
        }
        else if (direction == 2)
        {
            shift = (shift + 1U) & 7U;
            LED_PORT->ODR = rotate_4bit_window(sw, (8U - shift) & 7U);
        }
        else
        {
            LED_PORT->ODR = sw;
        }
        delay(40 * (4180 - speed));

        uint32_t total = 0;
        for (int i = 0; i < ADC_SAMPLES; i++)
        {
            ADC3->CR2 |= ADC_CR2_SWSTART; // Start conversion
            while (!(ADC3->SR & ADC_SR_EOC))
                ;              // Wait for conversion to complete
            total += ADC3->DR; // Read data
        }
        speed = total / ADC_SAMPLES;
    }
}

void init_buttons(void)
{
    // init GPIO
    // enable button ports
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOFEN;
    // clear modeR (set to input)
    GPIOE->MODER &= ~(3U << (BUTTON_PE6_PIN * 2U));
    GPIOF->MODER &= ~(3U << (BUTTON_PF8_PIN * 2U));
    GPIOF->MODER &= ~(3U << (BUTTON_PF9_PIN * 2U));
    // internal pull-up resistors, PUPDR = 01 means pull-up
    GPIOE->PUPDR &= ~(3U << (BUTTON_PE6_PIN * 2U));
    GPIOE->PUPDR |= (1U << (BUTTON_PE6_PIN * 2U));

    GPIOF->PUPDR &= ~(3U << (BUTTON_PF8_PIN * 2U));
    GPIOF->PUPDR |= (1U << (BUTTON_PF8_PIN * 2U));

    GPIOF->PUPDR &= ~(3U << (BUTTON_PF9_PIN * 2U));
    GPIOF->PUPDR |= (1U << (BUTTON_PF9_PIN * 2U));

    // EXTI init ---- UNDERSTAND ALL THIS BEFORE DEMO.
    // SYSCFG controls GPIO-to-EXTI routing.
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    /*
     * EXTI6 -> GPIOE
     *
     * EXTICR[1]:
     *   EXTI4 = bits 3:0
     *   EXTI5 = bits 7:4
     *   EXTI6 = bits 11:8
     *   EXTI7 = bits 15:12
     *
     * Port selection codes:
     *   GPIOA = 0
     *   GPIOB = 1
     *   GPIOC = 2
     *   GPIOD = 3
     *   GPIOE = 4
     *   GPIOF = 5
     */
    SYSCFG->EXTICR[1] &= ~(0xFU << 8U);
    SYSCFG->EXTICR[1] |= (0x4U << 8U);

    /*
     * EXTI8 -> GPIOF
     *
     * EXTICR[2]:
     *   EXTI8  = bits 3:0
     *   EXTI9  = bits 7:4
     */
    SYSCFG->EXTICR[2] &= ~(0xFU << 0U);
    SYSCFG->EXTICR[2] |= (0x5U << 0U);

    /*
     * EXTI9 -> GPIOF
     */
    SYSCFG->EXTICR[2] &= ~(0xFU << 4U);
    SYSCFG->EXTICR[2] |= (0x5U << 4U);

    /* Disable rising-edge triggers */
    EXTI->RTSR &= ~ALL_BUTTONS_MASK;

    /* Enable falling-edge triggers */
    EXTI->FTSR |= ALL_BUTTONS_MASK;
    /*
     * Clear stale pending flags before enabling the interrupt.
     *
     * EXTI pending bits are cleared by writing 1 to them.
     */
    EXTI->PR = ALL_BUTTONS_MASK;

    /*
     * Unmask EXTI lines 6, 8, and 9.
     */
    EXTI->IMR |= ALL_BUTTONS_MASK;

    /*
     * Lines 5 through 9 share this one NVIC interrupt.
     */
    NVIC_SetPriority(EXTI9_5_IRQn, 2U);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}
// handling functions for button presses
void Button_EXTI_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

// ISR for EXTI lines 5 through 9
void EXTI9_5_IRQHandler(void)
{
    // determine which button or buttons caused the interrupt
    uint32_t pending = EXTI->PR & ALL_BUTTONS_MASK;

    // Clear every pending button interrupt that was detected.
    // Write 1 to clear each EXTI pending flag.
    EXTI->PR = pending;

    if ((pending & BUTTON_PE6_MASK) != 0U)
    {
        // PE6 button pressed
        direction = 2U;
    }

    if ((pending & BUTTON_PF9_MASK) != 0U)
    {
        // PF9 button pressed
        direction = 1U;
    }

    if ((pending & BUTTON_PF8_MASK) != 0U)
    {
        // PF8 button pressed
        direction = 0U;
    }
}

uint8_t read_switches(void)
{
    return (SWITCH_PORT->IDR >> 8) & 0xF; // PC8..PC11 -> 4 bits
}