#include < stdio.h >

    void (* func[2])();

void function1(){
    Serial.println("function1");
}

void function2(){
    Serial.println("function2");
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.print("Serial started!!");
}

void loop() {
    // put your main code here, to run repeatedly:
    func[0] = & function1;
    func[1] = & function2;
    for (int i = 0; i < 2; i++ ) {
        func[i]();
    }
}
