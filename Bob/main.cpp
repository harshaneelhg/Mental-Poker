/*UDP Client Program using Berkley Sockets*/

// In our gameplay Bob acts as a client.
// Start Alice first then Bob.

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <cstdlib>
#include <algorithm>
#include<stdlib.h>
#include<time.h>
#include<string>
#include<iostream>

#define SERVER_PORT 11111
#define MAX_PENDING 5
#define MAX_LINE 256

using namespace std;

// Function Declarations.

int myrandom(int i);
bool isPrime(int n);
int gcd(int m, int n);
int generateKey();
int CalculateModular(int base, int pow, int mod);
int modInverse(int key);
string suit_of(int card);
string value_of(int x);


// Global variables.

int prime;
int phi;
int key_A;
int inv_A;
int alice_score=0;
int bob_score=0;

int main(int argc, char *argv[])
{
    printf("Starting Bob.....\n\n\n");
    struct sockaddr_in server;
    struct hostent *hp;
    char* buf=(char*) malloc(MAX_LINE*sizeof(char));
    int ret, n;
    int s;
    bool flag=true;

    // Initialize network requirements.
    // Includes: creating socket, binding program to
    //           localhost on a particular port.
    //           Bob tries to communicate on the same port
    //           as Alice. First we start Alice on SERVER_PORT
    //           then when we start Bob it communicates with
    //           Alice on the same port.

    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(0);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        perror("simplex-talk: UDP_socket error");
        exit(1);
    }

    if ((bind(s, (struct sockaddr *)&server, sizeof(server))) < 0)
    {
        perror("simplex-talk: UDP_bind error : Server");
        exit(1);
    }
    hp = gethostbyname( "localhost" );
    if( !hp )
    {
        fprintf(stderr, "Unknown host %s\n", "localhost");
        exit(1);
    }

    bzero( (char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    bcopy( hp->h_addr, (char *)&server.sin_addr, hp->h_length );
    server.sin_port = htons(SERVER_PORT);

    // Initializing deck of cards and shuffling it randomly.

    int cards[52];
    for(int i=0;i<52;i++)
        cards[i]=i;

    std::random_shuffle(&cards[0],&cards[52],myrandom);

    // Bob selects a prime number in range 50 to 100.
    // Later Bob sends this prime number to ALice.

    srand (time(NULL));
    int random = rand() % 50 + 50;

    while(!isPrime(random)){
        random = rand() % 50 + 50;
    }

    // prime variable stores randomly selected prime number.

    prime = random;
    phi = prime -1;
    printf("Selected prime...> %d\n",prime);

    // After selecting prime number calculate key for Bob and
    // calculate midular inverse for corresponding key.

    key_A = generateKey();
    printf("Generating key...\n");

    inv_A = modInverse(key_A);
    printf("Calculating inverse...\n");

    // Initialize variables and arrays required for gameplay.

    // variable 'itr' keeps track of iterations for sending bytes
    // to Alice.
    int itr=1;

    // variable 'itr1' keeps track of iterations for receiving bytes
    // from Alice.
    int itr1=1;

    // play_cards[] array is a collective set of Bob's and Alice's cards.
    int play_cards[10];

    // my_cards[] array stores Bob's cards.
    int my_cards[5];

    // played_card stores value for card played by Bob.
    int played_card=0;

    // alice_card stores value of card played by Alice.
    int alice_card=0;



    while(1)
    {
        // when itr=1 Bob sends prime number to Alice and sends
        // shuffled deck of 52 cards as well.

        if(itr==1)
        {
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            snprintf(buf, sizeof(buf),"%d",prime);
            n = strlen(buf);
            ret = sendto(s, buf, n, 0,(struct sockaddr *)&server, sizeof(server));
            if( ret != n)
            {
                fprintf( stderr, "Datagram Send error %d\n", ret );
                exit(1);
            }
            int idx=0;
            while(idx<52)
            {
                snprintf(buf, sizeof(buf),"%d",CalculateModular(cards[idx], key_A, prime));
                n = strlen(buf);
                ret = sendto(s, buf, n, 0,(struct sockaddr *)&server, sizeof(server));
                if( ret != n)
                {
                    fprintf( stderr, "Datagram Send error %d\n", ret );
                    exit(1);
                }
                idx++;
            }
            itr++;
        }

        // When itr=2 Bob sends Alice, her five cards.

        else if(itr==2)
        {
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            int idx=0;
            while(idx<5)
            {
                int x=0;
                x=play_cards[idx+5];
                snprintf(buf, sizeof(buf),"%d",x);
                n = strlen(buf);
                ret = sendto(s, buf, n, 0,(struct sockaddr *)&server, sizeof(server));
                if( ret != n)
                {
                    fprintf( stderr, "Datagram Send error %d\n", ret );
                    exit(1);
                }
                idx++;
            }
            itr++;
        }

        // itr=3 to itr=7 are the iterations for gameplay. During each
        // iteration Bob sends one card to Alice.
        // Whoever has better card in terms of value wins that hand.
        // Whoever wins more hands ultimately wins the game.

        else if(itr>=3&&itr<=7)
        {
            snprintf(buf, sizeof(buf),"%d",my_cards[itr-3]);
            n = strlen(buf);
            ret = sendto(s, buf, n, 0,(struct sockaddr *)&server, sizeof(server));
            if( ret != n)
            {
                fprintf( stderr, "Datagram Send error %d\n", ret );
                exit(1);
            }
            printf("Bob Plays: %s ",buf);
            cout<<" ("<<suit_of(atoi(buf))<<" "<<value_of(atoi(buf))<<")\n\n";
            played_card=atoi(buf);
            if(played_card>alice_card)
                bob_score++;
            else
                alice_score++;
            itr++;
        }

        // After all the iterations of gameplay break out of outer while loop
        // to exit out of program.

        else
            break;

        int ret1;
        socklen_t length=sizeof(server);

        // When itr1=1 Alice picks ten cards randomly from received deck of cards
        // from Bob and sends those ten cards to Bob. Bob receives those cards during this
        // iterartion. Before sending those cards,Alice encrypts last five cards with
        // her key. First five cards are Bob's cards so she leaves them as they are.

        if(itr1==1)
        {
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            int idx=0;
            while(idx<10)
            {   memset(buf, 0, sizeof(buf));
                ret1 = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&server, &length);

                if( ret1 < 0 )
                {
                    fprintf( stderr, "Send Error %d\n", ret );
                    exit(1);
                }
                buf[ret1] = 0;
                if(buf!="")
                {
                    play_cards[idx]=CalculateModular(atoi(buf),inv_A,prime);
                    if(idx<5)
                    {
                        my_cards[idx]= play_cards[idx];
                    }
                }
                idx++;
            }
            itr1++;
            for(int i=0;i<5;i++)
                cout<<"Bob's card #"<<i+1<<" : "<<my_cards[i]<<" ("<<suit_of(my_cards[i])<<" "<<value_of(my_cards[i])<<")\n";
        }

        // itr1=2 to itr1=7 are iterations for gameplay. During these iterations,
        // Bob waits for Alice to play her card and then Bob plays his card.
        // Bob receives one card from Alice in every iteration. So every iteration is
        // more like one round of hand.

        else if(itr1>=2&&itr<=7)
        {
            if(flag)
            {
                printf("============GAMEPLAY============\n");
                flag=false;
            }

            buf=(char*) malloc(MAX_LINE*sizeof(char));
            ret1 = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&server, &length);
            if( ret1 < 0 )
            {
                fprintf( stderr, "Send Error %d\n", ret );
                exit(1);
            }
            buf[ret1] = 0;
            printf("Alice plays: %s ",buf);
            cout<<" ("<<suit_of(atoi(buf))<<" "<<value_of(atoi(buf))<<")\n";
            alice_card=atoi(buf);
            itr1++;
        }

    }
    printf("================================\n");

    // Print final scores and declare winner.

    printf("Alice's score : %d\n",alice_score);
    printf("Bob's score : %d\n",bob_score);
    if(alice_score>bob_score)
        printf("Alice Wins !!!\n");
    else
        printf("Bob Wins !!!\n");

    return 0;
}

// myrandom() function returns random seed for random number generator.

int myrandom(int i)
{
    srand(time(NULL));
    return(rand()%i);
}

// isPrime() function checks if a number is prime.

bool isPrime(int n){
    if(n <= 3)
        return n > 1;
    else if(n%2 ==0  || n%3 ==0)
        return false;
    else {
        for(int i =5; i*i <=n; i+=6){
            if(n%i == 0 || n%(i+2) == 0 ){
                return false;
            }
        }
        return true;
    }
}

// gcd() function returns GCD of two numbers.

int gcd(int m, int n){
    if(n==0)
        return m;
    else
        return gcd(n, m%n);

}

// generateKey() returns the key such chat GCD of key and
// Euler-Totient value of prime number is 1.

int generateKey(){

    int i = rand() % 50;
    while(gcd(i, phi)!= 1){
        i = rand() % 50;
    }

    return i;
}

// CalculateModular() performs modular exponentiation and returns
// the answer.

int CalculateModular(int base, int pow, int mod){
    int p=1;
    int a1 = base % mod;
    while(pow > 0 ){
        if (pow % 2 != 0) {
            p *= a1;
            p = p % mod;
        }
        pow /= 2;
        a1 = (a1 * a1) % mod;
    }
    return p;
}

// modInverse() calculates modular inverse such that
// key*inverse mod (phi(prime)) = 1.

int modInverse(int key) {
    int a = key;
    a %= prime;
    for(int x = 1; x < prime; x++) {
        if((a*x) % phi == 1)
            return x;
    }


}

// suit_of() returns the suit a particular card belongs to.

string suit_of(int card)
{
    string s="";
    if(card/13==0)
    {
        s="Spade";
        return s;
    }
    else if(card/13==1)
    {
        s="Heart";
        return s;
    }
    else if(card/13==2)
    {
        s="Club";
        return s;
    }
    else
    {
        s="Diamond";
        return s;
    }
}

// value_of() returns the face value of a particular card.

string value_of(int x)
{
    string v = "";
    if(x%13==12)
    {
        v="Ace";
        return v;
    }
    if(x%13==0)
    {
        v="2";
        return v;
    }
    else if(x%13==1)
    {
        v="3";
        return v;
    }
    else if(x%13==2)
    {
        v="4";
        return v;
    }
    else if(x%13==3)
    {
        v="5";
        return v;
    }
    else if(x%13==4)
    {
        v="6";
        return v;
    }
    else if(x%13==5)
    {
        v="7";
        return v;
    }
    else if(x%13==6)
    {
        v="8";
        return v;
    }
    else if(x%13==7)
    {
        v="9";
        return v;
    }
    else if(x%13==8)
    {
        v="10";
        return v;
    }
    else if(x%13==9)
    {
        v="Jack";
        return v;
    }
    else if(x%13==10)
    {
        v="Queen";
        return v;
    }
    else if(x%13==11)
    {
        v="King";
        return v;
    }
    else
    {
        v="Ace";
        return v;
    }
}
