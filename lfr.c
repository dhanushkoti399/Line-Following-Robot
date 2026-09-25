#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)
#define GPIOA_CRL   (*(volatile uint32_t*)0x40010800)
#define GPIOA_IDR   (*(volatile uint32_t*)0x40010808)
#define GPIOA_ODR   (*(volatile uint32_t*)0x4001080C)
#define GPIOB_CRL   (*(volatile uint32_t*)0x40010C00)
#define GPIOB_IDR   (*(volatile uint32_t*)0x40010C08)
#define GPIOB_ODR   (*(volatile uint32_t*)0x40010C0C)
#define SYST_CSR    (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR    (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR    (*(volatile uint32_t*)0xE000E018)

static void delay_cycles(volatile uint32_t n){ while(n--) __asm volatile("nop"); }

static void gpio_init(void){
    RCC_APB2ENR |= (1u<<2) | (1u<<3);
    /* PA0,1,2,3,4,5,6 outputs push-pull 2MHz: CNF=00 MODE=10 => 0x2 */
    GPIOA_CRL = 0x22222222u;
    /* PB0..PB7 initially output push-pull 2MHz */
    GPIOB_CRL = 0x22222222u;
    GPIOA_ODR = (1u<<6);
    GPIOB_ODR = 0;
}

static void qtr_set_input(void){
    /* PB0..PB7 floating input: MODE=00 CNF=01 => 0x4 */
    GPIOB_CRL = 0x44444444u;
}

static void qtr_set_output(void){ GPIOB_CRL = 0x22222222u; }

static uint8_t qtr_read(void){
    uint32_t t[8]; uint32_t done=0;
    GPIOB_ODR |= 0xFFu;
    delay_cycles(500);
    qtr_set_input();
    for(uint32_t i=0;i<8;i++) t[i]=0;
    for(uint32_t n=0;n<12000 && done!=0xFFu;n++){
        uint32_t v=GPIOB_IDR & 0xFFu;
        for(uint32_t i=0;i<8;i++){
            if(!(done&(1u<<i))){
                if(!(v&(1u<<i))) { done|=(1u<<i); t[i]=n; }
            }
        }
    }
    qtr_set_output(); GPIOB_ODR=0;
    /* Darker/line response assumed: sensors returning longer time are 1. */
    uint8_t s=0;
    for(uint32_t i=0;i<8;i++) if(t[i]>2500) s|=(1u<<i);
    return s;
}

static void motors_stop(void){ GPIOA_ODR &= ~((1u<<0)|(1u<<1)|(1u<<3)|(1u<<4)); GPIOA_ODR &= ~((1u<<2)|(1u<<5)); GPIOA_ODR |= (1u<<6); }
static void forward(void){ GPIOA_ODR=(GPIOA_ODR & ~((1u<<0)|(1u<<1)|(1u<<3)|(1u<<4))) | (1u<<0)|(1u<<3)|(1u<<2)|(1u<<5); }
static void left(void){ GPIOA_ODR=(GPIOA_ODR & ~((1u<<0)|(1u<<1)|(1u<<3)|(1u<<4))) | (1u<<1)|(1u<<3)|(1u<<2)|(1u<<5); }
static void right(void){ GPIOA_ODR=(GPIOA_ODR & ~((1u<<0)|(1u<<1)|(1u<<3)|(1u<<4))) | (1u<<0)|(1u<<4)|(1u<<2)|(1u<<5); }

int main(void){
    gpio_init();
    while(1){
        uint8_t s=qtr_read();
        /* PB0 is leftmost, PB7 rightmost. */
        if(s==0){ forward(); }
        else if(s & 0x18){ forward(); }
        else if(s & 0x07){ left(); }
        else if(s & 0xE0){ right(); }
        else if(s & 0x0F){ left(); }
        else if(s & 0xF0){ right(); }
        else { motors_stop(); }
        delay_cycles(8000);
    }
}
