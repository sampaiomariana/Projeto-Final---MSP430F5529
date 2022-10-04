#include <msp430.h>
#include <stdint.h>

#define INT_MED 52428           // Intervalo de medi﷿﷿es usando o SMCLK(20 por segundo i.e uma a cada 0.05 segundos).
#define N_SMCLK 1048576         // SMCLK
#define RESET 4                 // Reseta as flags a cada

#define TRUE    1
#define FALSE   0

uint8_t S1_COMPARE = 0,S2_COMPARE = 0, reset = 0, reset_S1 = 0, reset_S2 = 0;
uint16_t S1_RISING_EDGE,S1_FALLING_EDGE,S2_RISING_EDGE,S2_FALLING_EDGE,S1_DIST,S2_DIST,S1_DIFF,S2_DIFF;
static unsigned int entrada = 0;
static unsigned int saida = 0;
static unsigned int contador = 0;
volatile unsigned int mostra_CCR;

void ta0_config(void);
void ta1_config(void);
void ta2_config(void);
void tb0_config(void);
void gpio_config(void);
unsigned int calc_dist(unsigned int eco1,unsigned int eco2);

void bt_str(char *vet);
void bt_char(char c);
void USCI_A0_config(void);





/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	USCI_A0_config();
	bt_str("Pronto para operar.\n");

	gpio_config();
	ta0_config();
	ta1_config();
	ta2_config();
	tb0_config();


	__enable_interrupt();
	while(1){

	    while(S1_COMPARE == 2){
	        S1_DIST = calc_dist(S1_RISING_EDGE, S1_FALLING_EDGE);
//	        S1_DIST /= 2;
	        if (S1_DIST > 0 && S1_DIST < 8){
                if(entrada == 1){
                    contador++;

                    entrada = 0;
                    P1OUT &= BIT0;          // LED vermelho apagado
                    P4OUT |= BIT7;          // LED verde aceso
                    bt_str("Verifica o sensor.\n");
                    bt_str("Entraram na sala.\n");
                    saida = 0;

                }
                else{
                    saida = 1;
                    reset_S1 = 0;
                }
	        }
	        reset_S1++;
            S1_COMPARE = 0;
	        if (reset_S1 == 50 && saida == 1){
	            saida = 0;
	            reset_S1 = 0;
	        }
	        else if(reset_S1 == 100){
	            reset_S1 = 0;
	        }
	    }
	    _delay_cycles(20);
        while (S2_COMPARE == 2){
            S2_DIST = calc_dist(S2_RISING_EDGE,S2_FALLING_EDGE);
//            S2_DIST /= 2;
            if (S2_DIST > 0 && S2_DIST < 8) {
                if (saida == 1) {
                    if (contador <= 0) {
                        contador = 0;
                        saida = 0;
                        entrada = 0;
                    } else {
                        contador--;
                        saida = 0;
                        P1OUT |= BIT0;         // Led vermelho aceso
                        P4OUT &= ~BIT7;         // Led verde apagado
                        bt_str("Verifica o sensor.\n");
                        bt_str("Sairam da sala.\n");
                        entrada = 0;
                    }
                } else {
                    entrada = 1;
                    reset_S2 = 0;
                }
            }
            reset_S2++;
            S2_COMPARE = 0;
            if(reset_S2 == 50 && entrada == 1){
                entrada = 0;
                reset_S2 = 0;
            }
            else if (reset_S2 == 100){
                reset_S2 = 0;
            }
        }
        _delay_cycles(20);
	}
}

// Configuracao do Timer A0 ( Trigger S1)
void ta0_config(void){
    TA0CTL = TASSEL__ACLK | ID__2 | MC__UP;

    TA0CCR0 = 3276;
    TA0CCR4 = 1638;

    TA0CCTL4 |= OUTMOD_3;
}

// Configuracao do Timer B0 ( Trigger S2 )
void tb0_config(void){
    TB0CTL = TASSEL__ACLK | MC__UP;

    TB0CCR0 = 6553;
    TB0CCR2 = 3276;

    TB0CCTL2 |= OUTMOD_3;
}

// Configuracao Timer 1 ( Medidor de duracao do Eco Sensor 1)
void ta1_config(void){
    TA1CTL = TASSEL__SMCLK | ID__2 | MC__CONTINOUS ;

    TA1CCTL1 = CM_3 | CCIS_0 | CAP | CCIE;
}

// Configucao do Timer 2 ( Medidor de duracao do Eco Sensor 2)
void ta2_config(){
    TA2CTL = TASSEL__SMCLK | ID__2 | MC__CONTINOUS ;

    TA2CCTL2 = CM_3 | CCIS_0 | CAP | CCIE;
}

// Configuracao dos IO
void gpio_config(void){
    P1DIR |= BIT5;              // P1.5 como saida
    P1SEL |= BIT5;              // P1.5 como timer(TA0.4)

    P7DIR |= BIT4;              // P7.4 como saida
    P7SEL |= BIT4;              // P7.4 como timer(TB0.2)

    P2DIR &= ~BIT0;             // P2.0 como entrada
    P2SEL |= BIT0;              // P2.0 como timer(TA1.1)

    P2DIR &= ~BIT5;              // P2.5 como entrada
    P2SEL |= BIT5;              // P2.5 como timer(TA2.2)

    P1DIR |= BIT0;              // P1.0 como saida
    P1OUT &= ~BIT0;
    P4DIR |= BIT7;              // P1.4 como saida
    P4OUT &= ~BIT7;
}

// Interrupcao do TA1(Echo S1)
#pragma vector = TIMER1_A1_VECTOR
__interrupt void TA1_GROUP_ISR(){
    switch(TA1IV){
    case TA1IV_TA1CCR1:
        if(S1_COMPARE == 0){
            S1_RISING_EDGE = TA1CCR1;
            S1_COMPARE = 1;
        }
        else if(S1_COMPARE == 1){
            S1_FALLING_EDGE = TA1CCR1;
            S1_COMPARE = 2;
        }
        break;
    default:
        break;
    }
}

// Interrupcao do TA2(Echo S2)
#pragma vector = TIMER2_A1_VECTOR
__interrupt void TA2_GROUP_ISR(){
    switch(TA2IV){
    case TA2IV_TA2CCR2:
        if(S2_COMPARE == 0){
            S2_RISING_EDGE = TA2CCR2;
            S2_COMPARE = 1;
        }
        else if(S2_COMPARE == 1){
            S2_FALLING_EDGE = TA2CCR2;
            S2_COMPARE = 2;
        }
        break;
    default:
        break;
    }
}

unsigned int calc_dist(unsigned int eco1,unsigned int eco2){
    volatile int valor = 0;
    volatile double calc = 0;
    volatile uint32_t diff = 0;

    if (eco1 > eco2){
        eco2 += 12585;
    }

    diff = (eco2 - eco1);
    diff *= 34300 ;
    diff >>= 20;
    valor = diff;


    return valor;
}

// Configurar USCI_A0
void USCI_A0_config(void){
    UCA0CTL1 = UCSWRST; //RST=1 para USCI_A0
    UCA0CTL0 = 0; //sem paridade, 8 bits, 1 stop, modo UART
    UCA0BRW = 3; //Divisor
    UCA0MCTL = UCBRS_3; //Modulador = 3 e UCOS=0
    P3SEL |= BIT4 | BIT3; //Disponibilizar pinos
    UCA0CTL1 = UCSSEL_1; //RST=0, ACLK
}

// Enviar uma string pela serial

void bt_str(char *vet){
    unsigned int i = 0;
    while (vet[i]!= '\0')
        bt_char(vet[i++]);
}

// Enviar um caracter pela serial

void bt_char(char c){
    while ((UCA0IFG & UCTXIFG) == 0); //Esperar TXIFG = 1
    UCA0TXBUF = c;
}
