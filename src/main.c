#include "stm32f4xx.h"

#define LED_PORT GPIOD

#define ANALOG_PIN 2
#define ANALOG_PORT GPIOC
#define ADC_CHANNEL 12 // ADC Channel for PC2
#define ADC_SAMPLES 16 // Number of samples for averaging

//buttons
#define BUTTON_PE6_PIN    6U
#define BUTTON_PF8_PIN    8U
#define BUTTON_PF9_PIN    9U
#define BUTTON_PE6_MASK   (1U << BUTTON_PE6_PIN)
#define BUTTON_PF8_MASK   (1U << BUTTON_PF8_PIN)
#define BUTTON_PF9_MASK   (1U << BUTTON_PF9_PIN)
#define ALL_BUTTONS_MASK  (BUTTON_PE6_MASK | \
                           BUTTON_PF8_MASK | \
                           BUTTON_PF9_MASK)

uint32_t speed = 0; // initialize global speed variable
uint8_t direction = 0;   // 0 = paused, 1 = shift left, 2 = shift right

void delay(volatile uint32_t count)
{
    while (count--);
}

int main(void)
{
    // enable clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    LED_PORT->MODER &= ~(0xFFFF);
    LED_PORT->MODER |= (0x5555);

    LED_PORT->ODR &= ~(0xFF);

    init_buttons();

    // configure LEDs as array

    while (1)
    {
        if (direction == 1)
        {
            // leds << 1
            //test
            LED_PORT->ODR |= (1 << 6);
        } else if (direction == 2)
        {
            // leds >> 1
            LED_PORT->ODR &= ~(1 << 6);
        } else {
            //nothing
        }
        delay(1000000 - speed);
    }
}

void init_buttons(void) {
    //init GPIO
    //enable button ports
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOFEN;
    // clear modeR (set to input)
    GPIOE->MODER &= ~(3U << (BUTTON_PE6_PIN * 2U));
    GPIOF->MODER &= ~(3U << (BUTTON_PF8_PIN * 2U));
    GPIOF->MODER &= ~(3U << (BUTTON_PF9_PIN * 2U));
    // internal pull-up resistors, PUPDR = 01 means pull-up
    GPIOE->PUPDR &= ~(3U << (BUTTON_PE6_PIN * 2U));
    GPIOE->PUPDR |=  (1U << (BUTTON_PE6_PIN * 2U));

    GPIOF->PUPDR &= ~(3U << (BUTTON_PF8_PIN * 2U));
    GPIOF->PUPDR |=  (1U << (BUTTON_PF8_PIN * 2U));
    
    GPIOF->PUPDR &= ~(3U << (BUTTON_PF9_PIN * 2U));
    GPIOF->PUPDR |=  (1U << (BUTTON_PF9_PIN * 2U));

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
    SYSCFG->EXTICR[1] |=  (0x4U << 8U);

    /*
     * EXTI8 -> GPIOF
     *
     * EXTICR[2]:
     *   EXTI8  = bits 3:0
     *   EXTI9  = bits 7:4
     */
    SYSCFG->EXTICR[2] &= ~(0xFU << 0U);
    SYSCFG->EXTICR[2] |=  (0x5U << 0U);

    /*
     * EXTI9 -> GPIOF
     */
    SYSCFG->EXTICR[2] &= ~(0xFU << 4U);
    SYSCFG->EXTICR[2] |=  (0x5U << 4U);
    
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
void Button_EXTI_Init(void) {
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