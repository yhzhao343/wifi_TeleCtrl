#include <IntervalTimer.h>
#include <aJSON.h>
#include <LiquidCrystal.h>
#define ARRAY_LENGTH(x)      (sizeof(x)/sizeof((x)[0]))
#define APPEND_CHILDREN(x)   children : x, length : ARRAY_LENGTH(x)
#define AJSON_IS_EMPTY(x)    ((x->type == aJson_Object) && ! (x->child))
#define IS_AUTO 15
#define DISPLAY_MOUNT 16
#define FOCUSER_DIR1 12
#define FOCUSER_DIR2 6
#define MANUAL_MODE_CHANGE 11
#define MANUAL_INTERVAL 0
#define DEBOUNCE_DELAY 50
#define LCD_REFRESH_INTERVAL 30
#define INVERVAL_REFRESH_INTERVAL 200
#define MOUNT_POS_REFRESH_INTERVAL 200
#define MOUNT_TRECKING_REFRESH_INTERVAL 2000
#define EXECUTE_MOUNT_CMD_INTERVAL 100
#define INTERVAL_LIMIT_1_LOWER 1500
#define INTERVAL_LIMIT_1_UPPER 50000
#define INTERVAL_LIMIT_2_LOWER 3000
#define INTERVAL_LIMIT_2_UPPER 100000
#define MUM_INTERVAL (4095.0/50.0)

LiquidCrystal lcd(18, 19, 20, 21, 22, 23);
IntervalTimer timer0;

const static boolean cycle[8][4] = {{true,false,false,false},
                                    {true,true,false,false},
                                    {false,true,false,false},
                                    {false,true,true,false},
                                    {false,false,true,false},
                                    {false,false,true,true},
                                    {false,false,false,true},
                                    {true,false,false,true}};
int cur_step = 0;
int step_mode = 1;
int manual_step_mode = 1;
int step_interval = 10000;
int stepper_motor_pins[] = {2,3,4,5};
char inputString[200];
int input_cursor = 0;
boolean stringComplete = false;

char inputString2[200];
int input_cursor2 = 0;
boolean stringComplete2 = false;
long last_mount_pos_update = 0;

char RA_BUFF[9];
char DEC_BUFF[9];
double cur_RA;
double cur_DEC;

long cur_time = 0;
long last_lcd_update = 0;

int manual_mode_change_btn_state = 1;
int last_manual_mode_change_btn_reading = 1;
long last_manual_model_debounceTime = 0;
int manual_mode_change_btn_reading = 0;

int manual_interval_raw = 0;
int last_manual_interval_raw = 0;
long last_manual_interval_update = 0;

boolean in_goto = false;

typedef enum {
    ctrl_object_node,
    ctrl_action_node,
    ctrl_parent_node
} ctrl_node_type;

typedef enum {
    IDLE,
    GET_RA_DEC,
    GET_TRACKING,
    SET_TRACKING,
    GOTO,
    GET_IN_GOTO,
    SLEW
} MOUNT_OPTION;

typedef struct coord {
    char (*ra) [6];
    char (*dec) [6];
} coord;

typedef struct mount_cmd {
    MOUNT_OPTION status;
    char* cmd;
} mount_cmd;

char mount_cmd_head = 0;
char mount_cmd_tail = 0;
long last_execute_mount_cmd = 0;

long last_get_tracking = 0;
char cur_tracking_mode = 0;

mount_cmd mount_cmd_buff[5] = {
    {
        status : IDLE,
        cmd : NULL
    },
    {
        status : IDLE,
        cmd : NULL
    },
    {
        status : IDLE,
        cmd : NULL
    },
    {
        status : IDLE,
        cmd : NULL
    },
    {
        status : IDLE,
        cmd: NULL
    }
};

typedef enum {
    INVALID_API_REQUEST,
    NO_MEMORY,
    NOT_IMPLEMENTED,
    INTERNAL_CTRL_FAULT,
    SET_FAIL,
    WRONG_SET_TYPE,
    OUT_OF_BOUND
} ctrl_err_type;

typedef void (*action)(void);
typedef int  (*get_func)(aJsonObject* request_child);
typedef int  (*validate_func)(aJsonObject* request_child);
typedef int  (*set_func)(aJsonObject* request_child, void* mystique);

typedef struct validate_set{
    validate_func validate;
    set_func set;
} validate_set;

typedef struct ctrl_node {
    const ctrl_node_type type;
    const char* string;
    get_func get;
    const validate_set* setter;
    const struct ctrl_node* children;
    const char length;
} ctrl_node;
static int validate_type(aJsonObject* request_child, char type);

static int get_focuser_mode(aJsonObject* request_child);
static int validate_focuser_mode(aJsonObject* request_child);
static int set_focuser_mode(aJsonObject* request_child, void* mystique);

static int get_focuser_interval(aJsonObject* request_child);
static int validate_focuser_interval(aJsonObject* request_child);
static int set_focuser_interval(aJsonObject* request_child, void* mystique);

static int get_telescope_tracking(aJsonObject* request_child);
static int get_telescope_RA(aJsonObject* request_child);
static int get_telescope_DEC(aJsonObject* request_child);
static int validate_telescope_tracking(aJsonObject* request_child);
static int set_telescope_tracking(aJsonObject* request_child, void* mystique);

static int validate_slew(aJsonObject* request_child);
static int set_slew(aJsonObject* request_child, void* mystique);

static int telescope_EQ_Coord_parent(aJsonObject* request_child);
static void int_to_hex(char (*hex_string) [6], int num) ;

static int get_telescope_in_goto (aJsonObject* request_child);
static int validate_telescope_EQ_Coord_RA(aJsonObject* request_child);
static int validate_telescope_EQ_Coord_DEC(aJsonObject* request_child);
static int set_telescope_EQ_Coord_RA(aJsonObject* request_child, void* mystique);
static int set_telescope_EQ_Coord_DEC(aJsonObject* request_child, void* mystique);

const static validate_set focuser_mode_setter = {
    validate : validate_focuser_mode,
    set : set_focuser_mode
};

const static validate_set focuser_interval_setter = {
    validate : validate_focuser_interval,
    set : set_focuser_interval
};

const static validate_set telescope_tracking_setter = {
    validate : validate_telescope_tracking,
    set : set_telescope_tracking
};

const static validate_set slew_setter = {
    validate : validate_slew,
    set : set_slew
};

const static validate_set telescope_EQ_Coord_RA_setter = {
    validate : validate_telescope_EQ_Coord_RA,
    set : set_telescope_EQ_Coord_RA
};

const static validate_set telescope_EQ_Coord_DEC_setter = {
    validate : validate_telescope_EQ_Coord_DEC,
    set : set_telescope_EQ_Coord_DEC
};
const static ctrl_node focuser_children[] = {
    {
        type : ctrl_action_node,
        string : "mode",
        get : get_focuser_mode,
        setter : &focuser_mode_setter,
        children : NULL,
        length : 0
    },
    {
        type : ctrl_action_node,
        string : "interval",
        get : get_focuser_interval,
        setter : &focuser_interval_setter,
        children : NULL,
        length : 0
    }
};
const static ctrl_node telescope_EQ_Coord_children[] = {
    {
        type : ctrl_action_node,
        string : "RA",
        get : get_telescope_RA,
        setter : &telescope_EQ_Coord_RA_setter,
        children : NULL,
        length : 0
    },
    {
        type : ctrl_action_node,
        string : "DEC",
        get : get_telescope_DEC,
        setter : &telescope_EQ_Coord_DEC_setter,
        children : NULL,
        length : 0
    },
};

const static ctrl_node telescope_children[] = {
    {
        type : ctrl_parent_node,
        string : "EQ_Coord",
        get : telescope_EQ_Coord_parent,
        setter : NULL,
        APPEND_CHILDREN(telescope_EQ_Coord_children)
    },
    {
        type : ctrl_action_node,
        string : "tracking",
        get : get_telescope_tracking,
        setter : &telescope_tracking_setter,
        children : NULL,
        length : 0
    },
    {
        type : ctrl_action_node,
        string : "in_goto",
        get : get_telescope_in_goto,
        setter : NULL,
        children: NULL,
        length: 0
    },
    {
        type : ctrl_action_node,
        string : "slew",
        get : NULL,
        setter : &slew_setter,
        children: NULL,
        length: 0
    }
};

const static ctrl_node root_children[] = {
    {
        type : ctrl_object_node,
        string : "focuser",
        get : NULL,
        setter : NULL,
        APPEND_CHILDREN(focuser_children)
    },
    {
        type : ctrl_object_node,
        string : "telescope",
        get : NULL,
        setter : NULL,
        APPEND_CHILDREN(telescope_children)
    },
};

const static ctrl_node root = {
    type : ctrl_object_node,
    string : "",
    get : NULL,
    setter : NULL,
    APPEND_CHILDREN(root_children)
};

static void parse_json(aJsonObject* request_parent, const ctrl_node* internal_parent, void* mystique) {
    if(!request_parent || ! internal_parent){
        return;
    }
    aJsonObject* request_child = request_parent -> child;
    while(request_child) {
        char* key = request_child -> name;
        int child_index = get_ctrl_node_child_ndx(internal_parent, key);
        if(child_index == -1){
            parsing_error_handle(request_child, INVALID_API_REQUEST);
            request_child = request_child -> next;
            continue;
        }
        const ctrl_node* internal_child = &((internal_parent -> children)[child_index]);
        if(AJSON_IS_EMPTY(request_child)){
            get_entire_object(request_child, internal_child);
        } else if(ctrl_action_node == internal_child -> type) {
            const validate_set* my_setter = internal_child -> setter;
            if(!my_setter){
                parsing_error_handle(request_child, NOT_IMPLEMENTED);
            } else {
                if(((validate_func)(my_setter -> validate))(request_child)){
                    if(!((set_func)(my_setter -> set))(request_child, mystique)) {
                        parsing_error_handle(request_child, SET_FAIL);
                    }
                }
            }
        } else if(ctrl_parent_node == internal_child -> type) {
            if(internal_child -> get) {
                ((get_func)(internal_child -> get))(request_child);
            } else {
                parsing_error_handle(request_child,  NOT_IMPLEMENTED);
            }
        } else if(ctrl_object_node == internal_child -> type) {
            parse_json(request_child, internal_child, NULL);
        } else {
            parsing_error_handle(request_child, INTERNAL_CTRL_FAULT);
        }
        request_child = request_child -> next;
    }

}


static void get_entire_object(aJsonObject* request_child, const ctrl_node* internal_parent) {
    switch (internal_parent -> type) {
        case ctrl_action_node:
            {
                if(!internal_parent -> get){
                    parsing_error_handle(request_child, NOT_IMPLEMENTED);
                } else {
                    ((get_func)(internal_parent -> get))(request_child);
                }
                break;
            }
        case ctrl_object_node:
        case ctrl_parent_node:
            {
                const ctrl_node* internal_children = internal_parent -> children;
                for(int i = 0; i < internal_parent -> length; i++){
                    const ctrl_node* current_internal_node = &(internal_children[i]);
                    aJsonObject* newObject = aJson.createObject();
                    if(!newObject){
                        parsing_error_handle(request_child, NO_MEMORY);
                        return;
                    }
                    aJson.addItemToObject(request_child, current_internal_node -> string, newObject);
                    get_entire_object(newObject, current_internal_node);
                }
                break;
            }
        default:
            {
                parsing_error_handle(request_child, INTERNAL_CTRL_FAULT);
                break;
            }
    }
    if(ctrl_action_node == internal_parent -> type){
        if(!internal_parent -> get){
            parsing_error_handle(request_child, NOT_IMPLEMENTED);
        } else {
            ((get_func)(internal_parent -> get))(request_child);
        }
    } else if (ctrl_object_node == internal_parent -> type){
        const ctrl_node* internal_children = internal_parent -> children;
        for(int i = 0; i < internal_parent -> length; i++){
            const ctrl_node* current_internal_node = &(internal_children[i]);
            aJsonObject* newObject = aJson.createObject();
            if(!newObject){
                parsing_error_handle(request_child, NO_MEMORY);
                return;
            }
            aJson.addItemToObject(request_child, current_internal_node -> string, newObject);
            get_entire_object(newObject, current_internal_node);
        }
    } else {
        parsing_error_handle(request_child, INTERNAL_CTRL_FAULT);
    }
}



static int get_ctrl_node_child_ndx(const ctrl_node* internal_parent, char* key) {
    const ctrl_node* children = internal_parent -> children;
    for (int i = 0; i < internal_parent -> length; ++i) {
        const ctrl_node* child = children + i;
        if (child) {
            if (strcmp((child -> string), key) == 0) {
                return i;
            }
        }
    }
    return -1;
}

char tracking_cmd[] = {0x54, 0x02};
char slew_cmd[] = {0x50, 0x02, 0x0f, 0x0f, 0xf, 0x00, 0x00, 0x00};
char goto_string[] = {0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '0', '0', 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '0', '0'};
void setup() {
    for(int i = 0; i < 4; i++){
        pinMode(stepper_motor_pins[i], OUTPUT);
    }
    pinMode(IS_AUTO, INPUT_PULLUP);
    pinMode(FOCUSER_DIR1, INPUT_PULLUP);
    pinMode(FOCUSER_DIR2, INPUT_PULLUP);
    pinMode(MANUAL_MODE_CHANGE, INPUT_PULLUP);
    pinMode(DISPLAY_MOUNT, INPUT_PULLUP);
    lcd.begin(16, 2);
    Serial.begin(9600);
    Serial2.begin(9600);
    analogReadResolution(12);
    timer0.begin(step, step_interval);
    manual_interval_raw = (int)((double)digitalRead(MANUAL_INTERVAL) / MUM_INTERVAL);
    last_manual_interval_raw = manual_interval_raw;
    // Serial.println(tracking_cmd);
    Serial2.print(tracking_cmd);
}

void loop() {
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      // add it to the inputString:
      inputString[input_cursor] = inChar;
      input_cursor++;
      // if the incoming character is a newline, set a flag
      // so the main loop can do something about it:
      if (inChar == '\n') {
        stringComplete = true;
      }
    }

    while(Serial2.available()) {
      char inChar = (char) Serial2.read();
      inputString2[input_cursor2] = inChar;
      input_cursor2++;
      if (inChar == '#') {
        stringComplete2 = true;
      }
    }

    cur_time = millis();
    if(cur_time - last_lcd_update > LCD_REFRESH_INTERVAL){
        lcd.clear();
        if(digitalRead(DISPLAY_MOUNT)) {
            lcd.setCursor(0,0);
            lcd.print(F("RA:"));
            lcd.setCursor(3, 0);
            lcd.print(cur_RA, 5);
            lcd.setCursor(13, 0);
            lcd.print(F("T:"));
            lcd.setCursor(15, 0);
            lcd.print((int)cur_tracking_mode);
            lcd.setCursor(0,1);
            lcd.print(F("DEC:"));
            lcd.setCursor(4, 1);
            lcd.print(cur_DEC, 5);
        } else {
            lcd.setCursor(0,0);
            lcd.print(F("mode:"));
            lcd.setCursor(5,0);
            lcd.print(step_mode);
            lcd.setCursor(0,1);
            lcd.print(F("INT:"));
            lcd.setCursor(4,1);
            lcd.print(step_interval);
        }
        last_lcd_update = cur_time;
    }

    cur_time = millis();
    if(cur_time - last_manual_interval_update > INVERVAL_REFRESH_INTERVAL){
        manual_interval_raw = (int)((double)analogRead(MANUAL_INTERVAL) / MUM_INTERVAL);
        if(last_manual_interval_raw != manual_interval_raw){
            if(1 == manual_step_mode){
                step_interval = INTERVAL_LIMIT_1_LOWER + (int)((double)manual_interval_raw * (double)(INTERVAL_LIMIT_1_UPPER - INTERVAL_LIMIT_1_LOWER)/MUM_INTERVAL);
            } else {
               step_interval = INTERVAL_LIMIT_2_LOWER + (int)((double)manual_interval_raw * (double)(INTERVAL_LIMIT_2_UPPER - INTERVAL_LIMIT_2_LOWER)/MUM_INTERVAL);
            }
            timer0.end();
            timer0.begin(step, step_interval);
            last_manual_interval_update = cur_time;
            last_manual_interval_raw = manual_interval_raw;
        }

    }

    if(cur_time - last_mount_pos_update > MOUNT_POS_REFRESH_INTERVAL){
        add_to_mount_queue(GET_RA_DEC, NULL);
        if(in_goto){
            add_to_mount_queue(GET_IN_GOTO, NULL);
        }
        last_mount_pos_update = cur_time;
    }
    if(cur_time - last_get_tracking > MOUNT_TRECKING_REFRESH_INTERVAL){
        add_to_mount_queue(GET_TRACKING, NULL);
        last_get_tracking = cur_time;
    }
    if(cur_time - last_execute_mount_cmd > EXECUTE_MOUNT_CMD_INTERVAL){
        execute_mount_cmd();
        last_execute_mount_cmd = cur_time;
    }

    if(!digitalRead(IS_AUTO)){
        // Serial.println(digitalRead(FOCUSER_DIR1));
        if(!digitalRead(FOCUSER_DIR1) && !digitalRead(FOCUSER_DIR2)){
            step_mode = 0;
        }else if(! digitalRead(FOCUSER_DIR1)){
            step_mode = manual_step_mode;
        }else if(! digitalRead(FOCUSER_DIR2)){
            step_mode = -manual_step_mode;
        } else {
            step_mode = 0;
        }

        manual_mode_change_btn_reading = digitalRead(MANUAL_MODE_CHANGE);
        if(manual_mode_change_btn_reading != last_manual_mode_change_btn_reading){
            last_manual_model_debounceTime = millis();
        }
        if((millis() - last_manual_model_debounceTime) > DEBOUNCE_DELAY){
            if(manual_mode_change_btn_reading != manual_mode_change_btn_state){
                manual_mode_change_btn_state = manual_mode_change_btn_reading;
                if(!manual_mode_change_btn_state){
                    if(1 == manual_step_mode){
                        manual_step_mode = 2;
                        step_interval = INTERVAL_LIMIT_2_LOWER + (int)((double)manual_interval_raw * (double)(INTERVAL_LIMIT_2_UPPER - INTERVAL_LIMIT_2_LOWER)/MUM_INTERVAL);
                    } else {
                        manual_step_mode = 1;
                        step_interval = INTERVAL_LIMIT_1_LOWER + (int)((double)manual_interval_raw * (double)(INTERVAL_LIMIT_1_UPPER - INTERVAL_LIMIT_1_LOWER)/MUM_INTERVAL);
                    }
                }
            }
        }
        last_manual_mode_change_btn_reading = manual_mode_change_btn_reading;
    }

    if (stringComplete) {
        stringComplete = false;
        aJsonObject* json = aJson.parse(inputString);
        if(json){
          Serial.println(F("valid_JSON"));
          parse_json(json, &root, NULL);
          char* text = aJson.print(json);
          Serial.println(text);
          aJson.deleteItem(json);
          free(text);
        } else {
          Serial.println(inputString);
        }
        while(input_cursor--) {
          inputString[input_cursor] =  0;
        }
        input_cursor = 0;
    }
    if(stringComplete2){
        stringComplete2 = false;
        parse_mount(inputString2);
        // Serial.println(inputString2);
        // Serial.println(input_cursor2);
        while(input_cursor2--) {
            inputString2[input_cursor2] = 0;
        }
        input_cursor2 = 0;
    }

}
static void parse_mount(char* msg) {
    char cur_cmd;
    if(0 == mount_cmd_head){
        cur_cmd = ARRAY_LENGTH(mount_cmd_buff) - 1;
    } else {
        cur_cmd = mount_cmd_head - 1;
    }
    MOUNT_OPTION cur_mount_option = mount_cmd_buff[cur_cmd].status;
    // Serial.printf("current_processing_cmd: %d msg: %s cur_mount_option: %d\n", cur_cmd, msg, cur_mount_option);
    switch (cur_mount_option) {
        case GET_RA_DEC:
            if(18 == input_cursor2){
                for(int i = 0; i < 6; i++){
                    RA_BUFF[i] = msg[i];
                    DEC_BUFF[i] = msg[i + 9];
                }
                cur_RA = (double)strtol(RA_BUFF, NULL, 16) * 360.0 / 16777216.0;
                cur_DEC = (double)strtol(DEC_BUFF, NULL, 16) * 360.0 / 16777216.0;
            }
            break;
        case GET_TRACKING:
            if(2 == input_cursor2){
                cur_tracking_mode = msg[0];
                break;
            }
            break;
        case GET_IN_GOTO:
            if('1' == msg[0]){
                in_goto = true;
            } else {
                in_goto = false;
            }
        break;
        default:
            break;
    }
    mount_cmd_buff[cur_cmd].status = IDLE;
    mount_cmd_buff[cur_cmd].cmd = NULL;
}
void step() {
    if(step_mode == 0){
        return;
    }
    const boolean* step_val = cycle[cur_step];
    for(int i = 0; i < 4; i++){
        digitalWrite(stepper_motor_pins[i], step_val[i]);
    }
    if(step_mode == 1 || step_mode == -1 || cur_step % 2) {
        cur_step = cur_step + step_mode + 8;
    } else {
        cur_step = cur_step + step_mode / 2 + 8;
    }

    cur_step %= 8;
}

static void parsing_error_handle(aJsonObject* request_child, ctrl_err_type error) {
    request_child -> type = aJson_String;
    switch (error) {
        case INVALID_API_REQUEST :
            request_child -> valuestring = strdup((char*) F("parsing error: invalid API request"));
            break;
        case NO_MEMORY:
            request_child -> valuestring = strdup((char*) F("No memory! T_T "));
            break;
        case NOT_IMPLEMENTED:
            request_child -> valuestring = strdup((char*) F("Not implemented yet."));
            break;
        case INTERNAL_CTRL_FAULT:
            request_child -> valuestring = strdup((char*) F("Internal ctrl node type wrong"));
            break;
        case SET_FAIL:
            request_child -> valuestring = strdup((char*) F("Set failed"));
            break;
        case WRONG_SET_TYPE:
            request_child -> valuestring = strdup((char*) F("Set failed: wrong input type"));
            break;
        case OUT_OF_BOUND:
            request_child -> valuestring = strdup((char*) F("Set failed: winput out of bound"));
            break;
        default:
            break;
    }
}

static int telescope_EQ_Coord_parent(aJsonObject* request_child) {
    const ctrl_node* internal_child = telescope_children + 0;
    char ra[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    char dec[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    coord my_coord = {&ra, &dec};
    parse_json(request_child, internal_child, &my_coord);
    if((my_coord.ra)[0] && (my_coord.dec)[0]){
        for(int i = 0; i < 6; i++){
            goto_string[i + 1] = ((char*)(my_coord.ra))[i];
            goto_string[i + 9] = ((char*)(my_coord.dec))[i];
        }
        for(int i = 0; i < 18; i++){
            Serial.write(goto_string[i]);
        }
        Serial.println();
        add_to_mount_queue(GOTO, NULL);
        in_goto = true;
        return 1;
    }
    return 0;
}

static int get_focuser_mode(aJsonObject* request_child) {
    request_child -> type = aJson_Int;
    request_child -> valueint = step_mode;
    return 1;
}

static int get_focuser_interval(aJsonObject* request_child) {
    request_child -> type = aJson_Int;
    request_child -> valueint = step_interval;
    return 1;
}

static int validate_type(aJsonObject* request_child, char type) {
    if(!(type == request_child -> type)){
        parsing_error_handle(request_child, WRONG_SET_TYPE);
        return 0;
    }
    return 1;
}

static int get_telescope_tracking(aJsonObject* request_child) {
    request_child -> type = aJson_Int;
    request_child -> valueint = (int)cur_tracking_mode;
    return 1;
}

static int validate_telescope_tracking(aJsonObject* request_child) {
    if(! validate_type(request_child, aJson_Int)){
        return 0;
    }
    int tracking_mode = request_child -> valueint;
    if(tracking_mode < 0 || tracking_mode > 3){
        parsing_error_handle(request_child, OUT_OF_BOUND);
        return 0;
    }
    return 1;
}
static int set_telescope_tracking(aJsonObject* request_child, void* mystique) {
    tracking_cmd[1] = request_child -> valueint;
    add_to_mount_queue(SET_TRACKING, NULL);
    return 1;
}


static int get_telescope_RA(aJsonObject* request_child) {
    request_child -> type = aJson_Float;
    request_child -> valuefloat = cur_RA;
    return 1;
}

static int get_telescope_in_goto (aJsonObject* request_child) {
    request_child -> type = aJson_Boolean;
    request_child -> valuebool = in_goto;
    return 1;
}

static int get_telescope_DEC(aJsonObject* request_child) {
    request_child -> type = aJson_Float;
    request_child -> valuefloat = cur_DEC;
    return 1;
}

static int validate_focuser_mode(aJsonObject* request_child) {
    if(! validate_type(request_child, aJson_Int)){
        return 0;
    }
    int num = request_child -> valueint;
    if(num > 2 || num < -2){
        parsing_error_handle(request_child, OUT_OF_BOUND);
        return 0;
    }
    return 1;
}
static int set_focuser_mode(aJsonObject* request_child, void* mystique) {
    step_mode = request_child -> valueint;
    return 1;
}
static int validate_telescope_EQ_Coord_RA(aJsonObject* request_child) {
    if(! validate_type(request_child, aJson_Float)){
        return 0;
    }
    double num = request_child -> valuefloat;
    if(num < 0.0 || num > 360.0){
        parsing_error_handle(request_child, OUT_OF_BOUND);
        return 0;
    }
    return 1;
}
static int validate_telescope_EQ_Coord_DEC(aJsonObject* request_child) {
    if(! validate_type(request_child, aJson_Float)){
        return 0;
    }
    double num = request_child -> valuefloat;
    if(num < 0.0 || num > 90.0){
        parsing_error_handle(request_child, OUT_OF_BOUND);
        return 0;
    }
    return 1;
}

const static char hex_map[] = "0123456789ABCDEF";

static void int_to_hex(char (*hex_string) [6], int num) {
    for(int i = 5; i > -1; i--){
        ((char*)hex_string)[i] = hex_map[num & 0xf];
        num = num >> 4;
    }
}
static int set_telescope_EQ_Coord_RA(aJsonObject* request_child, void* mystique) {
   int_to_hex(((coord*)mystique) -> ra, (int)((request_child -> valuefloat)/360.0 * 16777216));
    return 1;
}

static int set_telescope_EQ_Coord_DEC(aJsonObject* request_child, void* mystique) {
    int_to_hex(((coord*)mystique) -> dec, (int)((request_child -> valuefloat)/360.0 * 16777216));
    return 1;
}

static int validate_goto(aJsonObject* request_child) {
    return  1;
}

static int validate_slew(aJsonObject* request_child) {
    return 1;
}

static int set_slew(aJsonObject* request_child, void* mystique) {
    char* string = request_child -> valuestring;
    char axis = string[0] - 49;
    char dir = string[1] - 49;
    char vel = string[2] - '0';
    slew_cmd[2] = axis;
    slew_cmd[3] = dir;
    slew_cmd[4] = vel;
    add_to_mount_queue(SLEW, NULL);
    return 1;
}
static int validate_focuser_interval(aJsonObject* request_child) {
    if(!validate_type(request_child, aJson_Int)){
        return 0;
    }
    int interval = request_child -> valueint;
    if((1 == step_mode) || (-1 == step_mode)){
        if(interval < INTERVAL_LIMIT_1_LOWER || interval > INTERVAL_LIMIT_1_UPPER){
            parsing_error_handle(request_child, OUT_OF_BOUND);
            return 0;
        }
    } else if ((2 == step_mode) || (-2 == step_mode)) {
        if(interval < INTERVAL_LIMIT_2_LOWER || INTERVAL_LIMIT_2_UPPER > 100000){
            parsing_error_handle(request_child, OUT_OF_BOUND);
            return 0;
        }
    }
    return 1;
}
static int set_focuser_interval(aJsonObject* request_child, void* mystique) {
    step_interval = request_child -> valueint;
    timer0.end();
    if(!timer0.begin(step, step_interval)) {
        return 0;
    }
    return 1;
}

static void add_to_mount_queue(MOUNT_OPTION option, char* cmd) {
    switch (option) {
        case GET_RA_DEC:
          cmd = (char*)(F("e"));
          break;
        case GET_TRACKING:
          cmd = ((char*)(F("t")));
           break;
        case SET_TRACKING:
            cmd = tracking_cmd;
            break;
        case GOTO:
            cmd = goto_string;
            // Serial.println(goto_string);
            break;
        case SLEW:
            cmd = slew_cmd;
            break;
        case GET_IN_GOTO:
            cmd = ((char*)(F("L")));
            break;
        default:
            break;
          // do something
    }
    mount_cmd_buff[mount_cmd_tail].status = option;
    mount_cmd_buff[mount_cmd_tail].cmd = cmd;
    // Serial.printf("mount_cmd_tail: %d command sent: %s\n", mount_cmd_tail, cmd);
    mount_cmd_tail = (mount_cmd_tail + 1) % ARRAY_LENGTH(mount_cmd_buff);
}

static void execute_mount_cmd() {
    if(IDLE != mount_cmd_buff[mount_cmd_head].status){
        switch (mount_cmd_buff[mount_cmd_head].status) {
            case SET_TRACKING:
                // Serial.println("set ");
                for(int i = 0; i < ARRAY_LENGTH(tracking_cmd); i++){
                    // Serial.printf("char sent: %d\n", (int)(mount_cmd_buff[mount_cmd_head].cmd[i]));
                    Serial2.write(mount_cmd_buff[mount_cmd_head].cmd[i]);
                }
                break;
            case SLEW:
                for(int i = 0; i < ARRAY_LENGTH(slew_cmd); i++){
                    Serial2.write(mount_cmd_buff[mount_cmd_head].cmd[i]);
                }
                break;
            default:
                Serial2.print(mount_cmd_buff[mount_cmd_head].cmd);
                break;
        }
        mount_cmd_head = (mount_cmd_head + 1) % ARRAY_LENGTH(mount_cmd_buff);

    }
}