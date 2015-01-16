/*UDP Server Program using Berkley Sockets*/

// In our gameplay Alice serves as a server.
// Start Alice first by running this project.

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <cstdlib>
#include<time.h>
#include<algorithm>
#include<string>
#include<iostream>
#define SERVER_PORT 11111
#define MAX_LINE 256

using namespace std;

// Function Declarations.

bool isPrime(int n);
int gcd(int m, int n);
int generateKey();
int CalculateModular(int base, int pow, int mod);
int modInverse(int key);
void pick_ten_cards(int cards[], int arr[]);
int myrandom(int i);
string suit_of(int card);
string value_of(int x);

// Global variables.

int prime;
int phi;
int key_A;
int inv_A;
int bob_score=0;
int alice_score=0;

int main (int argc, char * argv[])
{
    printf("Starting Alice.....\n\n\n");
    struct hostent *hp;
    struct sockaddr_in server, client;
    char* buf=(char*) malloc(MAX_LINE*sizeof(char));
    int s, ret;
    socklen_t length=0;
    bool flag=true;

    // Initialize network requirements.
    // Includes: creating socket, binding program to
    //           localhost on a particular port.

    hp = gethostbyname("localhost");
    if (!hp)
    {
        fprintf(stderr,"simplex-talk:Unknown host: %s\n",hp);
        exit(1);
    }

    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&server.sin_addr,hp->h_length);
    server.sin_port = htons(SERVER_PORT);

    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("simplex_talk: socket error");
        exit(1);
    }

    ret = bind(s, (struct sockaddr *)&server, sizeof(server));
    if( ret < 0)
    {
        fprintf( stderr, "Bind Error: can't bind local address");
        exit(1);
    }

    length = sizeof(client);

    // Initialize variables and arrays required for gameplay.

    // variable 'itr' keeps track of iterations for receiving bytes
    // from client.
    int itr=1;

    // variable 'itr1' keeps track of iterations for sending bytes
    // to client.
    int itr1=1;

    // cards[] array stores shuffled deck of cards received from client.
    int cards[52];

    // my_cards[] array stores Alice's cards.
    int my_cards[5];

    // played_card stores value for card played by Alice.
    int played_card=0;

    // bob_card stores value of card played by Bob.
    int bob_card=0;


    while(1)
    {
        // when itr=1 Alice receives prime number from Bob and receives
        // shuffled deck of 52 cards as well.
        if(itr==1)
        {
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            ret = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&client, &length);

            if( ret < 0 )
            {
                fprintf( stderr, "Send Error %d\n", ret );
                exit(1);
            }
            buf[ret] = 0;
            printf( "Receiving prime...> %s\n", buf);
            prime=atoi(buf);
            phi=prime-1;
            key_A = generateKey();
            inv_A = modInverse(key_A);
            printf( "Receiving shuffled deck of cards...\n");
            int idx=0;
            while(idx<52)
            {
                ret = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&client, &length);

                if( ret < 0 )
                {
                    fprintf( stderr, "Send Error %d\n", ret );
                    exit(1);
                }
                buf[ret] = 0;
                if(buf!="")
                {
                    cards[idx]=atoi(buf);
                    idx++;
                }
            }
            itr++;
        }

        // When itr=2 Alice receives her five cards from bob which she decrypts
        // using inverse modular function.

        else if(itr==2)
        {
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            int idx=0;
            while(idx<5)
            {
                ret = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&client, &length);

                if( ret < 0 )
                {
                    fprintf( stderr, "Send Error %d\n", ret );
                    exit(1);
                }
                buf[ret] = 0;
                if(buf!="")
                {
                    my_cards[idx]=CalculateModular(atoi(buf),inv_A,prime);
                    idx++;
                }
            }
            itr++;
            for(int i=0;i<5;i++)
            {
                cout<<"Alice's card #"<<i+1<<" : "<<my_cards[i]<<" ("<<suit_of(my_cards[i])<<" "<<value_of(my_cards[i])<<")\n";
            }

        }

        // itr=3 to itr=7 are the iterations for gameplay. During each
        // iteration Alice receives one card from Bob.
        // Whoever has better card in terms of value wins that hand.
        // Whoever wins more hands ultimately wins the game.

        else if(itr>=3&&itr<=7)
        {
            ret = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&client, &length);

            if( ret < 0 )
            {
                fprintf( stderr, "Send Error %d\n", ret );
                exit(1);
            }
            buf[ret] = 0;
            printf( "Bob plays: %s ", buf);
            cout<<" ("<<suit_of(atoi(buf))<<" "<<value_of(atoi(buf))<<")\n\n";
            bob_card=atoi(buf);

            // Updating scores on the basis of value of the card.

            if(played_card>bob_card)
                alice_score++;
            else
                bob_score++;
            itr++;
        }

        // After all the iterations of gameplay break out of outer while loop
        // to exit out of program.

        else
            break;

        int ret1,n;

        // When itr1=1 Alice picks ten cards randomly from received deck of cards
        // from Bob and sends those ten cards to Bob. Before sending those cards,
        // Alice encrypts last five cards with her key. First five cards are Bob's
        // cards so she leaves them as they are.

        if(itr1==1)
        {
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            int idx=0;
            int ret[10];
            pick_ten_cards(cards,ret);
            while(idx<10)
            {
                int x=0;

                // Leave first five cards as they are and encrypt last five cards.

                if(idx<5)
                    x=ret[idx];
                else
                    x=CalculateModular(ret[idx], key_A, prime);
                snprintf(buf, sizeof(buf),"%d",x);
                n = strlen(buf);
                ret1 = sendto(s, buf, n, 0,(struct sockaddr *)&client, sizeof(client));
                if( ret1 != n)
                {
                    fprintf( stderr, "Datagram Send error %d\n", ret );
                    exit(1);
                }
                idx++;
            }
            itr1++;
        }

        // itr1=2 to itr1=6 are iterations for gameplay. During these iterations,
        // Alice sends her cards to Bob one by one and waits for Bob to play after
        // she plays one card.

        else if(itr1>=2&&itr1<=6)
        {
            if(flag)
            {
                printf("============GAMEPLAY============\n");
                flag=false;
            }
            buf=(char*) malloc(MAX_LINE*sizeof(char));
            snprintf(buf, sizeof(buf),"%d",my_cards[itr1-2]);
            n = strlen(buf);
            ret1 = sendto(s, buf, n, 0,(struct sockaddr *)&client, sizeof(client));
            printf( "Alice Plays : %s ", buf);
            cout<<" ("<<suit_of(atoi(buf))<<" "<<value_of(atoi(buf))<<")\n";
            played_card=atoi(buf);
            if( ret1 != n)
            {
                fprintf( stderr, "Datagram Send error %d\n", ret );
                exit(1);
            }

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
    printf("Generating key...\n");
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
    printf("Calculating inverse...\n");
    int a = key;
    a %= prime;
    for(int x = 1; x < prime; x++) {
        if((a*x) % phi == 1){
            return x;
        }
    }
}

// pick_ten_cards() randomly picks up ten cards from deck of
// 52 shuffled cards and stores the result in an array.

void pick_ten_cards(int cards[],int arr[])
{

    random_shuffle(&cards[0],&cards[52],myrandom);
    for(int i=0;i<10;i++)
    {
        arr[i]=cards[i];
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
