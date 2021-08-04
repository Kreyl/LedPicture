import sys
import csv
import os.path
import serial
import Usbhost
import pygame
from pygame.locals import *

port = Usbhost.get_device_port()
if not port:
    print("device is not connected")
ser = serial.Serial(port, baudrate=115200, timeout=0.1)
IMAGE_NAME = "beleriand.jpg"
PIXELS_X = 20
PIXELS_Y = 28
pixels_arr = []
INIT_W = 540
INIT_H = 700

colors = []
with open("colors.csv", "r") as f_obj:
    reader = csv.reader(f_obj)
    for row in reader:
        colors.append([int(item) for item in row])
colors.append([0,0,0,0])
 
pygame.init()
fps = 60
fpsClock = pygame.time.Clock()

IMAGE_SURF = pygame.image.load(IMAGE_NAME)
ERASER_SURF = pygame.image.load("eraser.png")
CLEAR_SURF = pygame.image.load("clear.png")
SENDALL_SURF = pygame.image.load("SendAll.png")
 
width, height = int(IMAGE_SURF.get_width() * 1.1), int(IMAGE_SURF.get_height() * 1.1)
WORK_ZONE = pygame.Surface((width, height))
WORK_ZONE.fill((255, 255, 255))
WORK_ZONE.blit(IMAGE_SURF, (0, 0))
PROPORTION = width/height
PIXEL_WIDTH = width / 1.1 / PIXELS_X
PIXEL_HEIGHT = height / 1.1 / PIXELS_Y
PALETTE_ITEM_WIDTH = width * (0.1 / 1.1)
PALETTE_ITEM_HEIGHT = height / len(colors)
BUTTON_WIDTH = width / 1.1 / 2
BUTTON_HEIGHT = height * (0.1 / 1.1)

prop = CLEAR_SURF.get_width()/CLEAR_SURF.get_height()
clear_w_new = int(min(BUTTON_WIDTH, BUTTON_HEIGHT * prop))
clear_scale_factor = clear_w_new / CLEAR_SURF.get_width()
clear_h_new = int(CLEAR_SURF.get_height() * clear_scale_factor)
WORK_ZONE.blit(pygame.transform.scale(CLEAR_SURF, (clear_w_new, clear_h_new)), (0, height - BUTTON_HEIGHT))
prop = SENDALL_SURF.get_width()/SENDALL_SURF.get_height()
sendall_w_new = int(min(BUTTON_WIDTH, BUTTON_HEIGHT * prop))
sendall_scale_factor = sendall_w_new / SENDALL_SURF.get_width()
sendall_h_new = int(SENDALL_SURF.get_height() * sendall_scale_factor)
WORK_ZONE.blit(pygame.transform.scale(SENDALL_SURF, (sendall_w_new, sendall_h_new)), (BUTTON_WIDTH, height - BUTTON_HEIGHT))
pygame.draw.rect(WORK_ZONE, (0,0,0), pygame.Rect(0, height - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT), 3)
pygame.draw.rect(WORK_ZONE, (0,0,0), pygame.Rect(BUTTON_WIDTH, height - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT), 3)

PIXELS_SURF = pygame.Surface((width, height), SRCALPHA)
PIXELS_SURF.fill((0,0,0,0))
for y in range(len(colors)):
    pygame.draw.rect(PIXELS_SURF, colors[y], pygame.Rect(width - PALETTE_ITEM_WIDTH, y * PALETTE_ITEM_HEIGHT, PALETTE_ITEM_WIDTH, PALETTE_ITEM_HEIGHT))
prop = ERASER_SURF.get_width()/ERASER_SURF.get_height()
eraser_w_new = int(min(PALETTE_ITEM_WIDTH, PALETTE_ITEM_HEIGHT * prop))
eraser_scale_factor = eraser_w_new / ERASER_SURF.get_width()
eraser_h_new = int(ERASER_SURF.get_height() * eraser_scale_factor)
PIXELS_SURF.blit(pygame.transform.scale(ERASER_SURF, (eraser_w_new, eraser_h_new)), (width - PALETTE_ITEM_WIDTH, (len(colors) - 1) * PALETTE_ITEM_HEIGHT))

RECT_SURF = pygame.Surface((width, height), SRCALPHA)
current_color = len(colors) - 1

def draw_palette_rect(current_color):
    RECT_SURF.fill((0,0,0,0))
    pygame.draw.rect(RECT_SURF, (255,0,0,255), pygame.Rect(width - PALETTE_ITEM_WIDTH, current_color * PALETTE_ITEM_HEIGHT, PALETTE_ITEM_WIDTH, PALETTE_ITEM_HEIGHT), 3)
draw_palette_rect(current_color)
'''def draw_pixels():
    for y in range(len(pixels_arr)):
        for x in range(len(pixels_arr[y])):
            pygame.draw.rect(
                WORK_ZONE,
                colors[pixels_arr[y][x]],
                (PIXEL_WIDTH * x, PIXEL_HEIGHT * y, PIXEL_WIDTH, PIXEL_HEIGHT)
            )'''
def get_pixel(pos, scale_factor):
    return (int(pos[0] / scale_factor // PIXEL_WIDTH),
            int(pos[1] / scale_factor // PIXEL_HEIGHT))
            
def get_color(pos, scale_factor):
    if (width - PALETTE_ITEM_WIDTH) <= (pos[0] / scale_factor) <= width:
        return int(pos[1] / scale_factor // PALETTE_ITEM_HEIGHT)
    return -1
    
def get_button(pos, scale_factor):
    if (height - BUTTON_HEIGHT) <= (pos[1] / scale_factor) <= height:
        if (pos[0] / scale_factor) < BUTTON_WIDTH:
            return 1
        if BUTTON_WIDTH < (pos[0] / scale_factor) < BUTTON_WIDTH*2:
            return 2

screen = pygame.display.set_mode((INIT_W, INIT_H), RESIZABLE)
w_new = int(min(INIT_W, INIT_H * PROPORTION))
scale_factor = w_new / width
h_new = int(height * scale_factor)
resized_surf = pygame.transform.scale(WORK_ZONE, (w_new, h_new))

 
def sendColor(x, y, R, G, B):
    #time.sleep(0.1)
    while ser.inWaiting() > 0:
        ser.readall()
    Cmd: str = 'SetPix %d %d, %d %d %d' % (x, y, R,G,B)
    print(Cmd)
    bCmd: bytes = bytes(Cmd + '\r\n', encoding='utf-8')
    for i in range(4):
        ser.write(bCmd)
        reply: str = ser.readline().decode('utf-8').strip()
        print(reply)
        if reply.startswith("Ok"):
            return True   
    print("error sending command")
    return False  

def DrawColor(x, y, color):
    pixels_arr[y][x] = color
    with open("save.csv", "w", newline="") as f_obj:
        writer = csv.writer(f_obj)
        for i in range(PIXELS_Y):
            writer.writerow([str(val) for val in pixels_arr[i]])
    pygame.draw.rect(
        PIXELS_SURF,
        colors[color],
        (PIXEL_WIDTH * x, PIXEL_HEIGHT * y, PIXEL_WIDTH, PIXEL_HEIGHT)
    ) 

def sendAndDrawColor(x, y, color):
    if sendColor(x, y, colors[color][0], colors[color][1], colors[color][2]):
        DrawColor(x, y, color)

if os.path.isfile("save.csv"):
    for y in range(PIXELS_Y):
        pixels_arr.append([0]*PIXELS_X)
    with open("save.csv", "r") as f_obj:
        reader = csv.reader(f_obj)
        y = 0
        for row in reader:
            for x in range(len(row)):
                sendAndDrawColor(x, y, int(row[x]))
            y += 1
else:
    with open("save.csv", "w") as f_obj:
        writer = csv.writer(f_obj)
        for y in range(PIXELS_Y):
            pixels_arr.append([len(colors) - 1] * PIXELS_X)
            writer.writerow([str(len(colors) - 1)] * PIXELS_X)


while True:
  
    for event in pygame.event.get():
        if event.type == QUIT:
            pygame.quit()
            sys.exit()
        if event.type == VIDEORESIZE:
            w_new = int(min(event.w, event.h * PROPORTION))
            scale_factor = w_new / width
            h_new = int(height * scale_factor)
            resized_surf = pygame.transform.scale(WORK_ZONE, (w_new, h_new))
        if event.type == MOUSEBUTTONDOWN:
            res = get_button(event.pos, scale_factor)
            if res:
                if res == 1:
                    if sendColor(255, 0, 0,0,0): # send CLS, x=255 is a Magic Number: fill with mentioned RGB
                        # Remove pixels from picture if success
                        for y in range(PIXELS_Y):
                            for x in range(PIXELS_X):
                                if pixels_arr[y][x] != len(colors) - 1:
                                    DrawColor(x, y, len(colors) - 1)
                if res == 2:
                    for y in range(PIXELS_Y):
                        for x in range(PIXELS_X):
                            if pixels_arr[y][x] != len(colors) - 1:
                                sendAndDrawColor(x, y, pixels_arr[y][x])
            color = get_color(event.pos, scale_factor)
            if len(colors) > color > -1:
                current_color = color
                draw_palette_rect(color)
            x, y = get_pixel(event.pos, scale_factor)
            if y < PIXELS_Y and x < PIXELS_X:
                sendAndDrawColor(x, y, current_color)
    screen.fill((0,0,0))
    screen.blit(resized_surf, (0, 0))
    screen.blit(pygame.transform.scale(PIXELS_SURF, (w_new, h_new)), (0,0))
    screen.blit(pygame.transform.scale(RECT_SURF, (w_new, h_new)), (0,0))
    pygame.display.flip()
    fpsClock.tick(fps)