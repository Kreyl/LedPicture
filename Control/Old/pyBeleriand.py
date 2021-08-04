import sys
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
INIT_W = 550
INIT_H = 700

colors = []
colors.append([0,255,0,100])
colors.append([255,0,0,100])
colors.append([180,180,180,100])
colors.append([0,0,0,0])

 
pygame.init()
fps = 60
fpsClock = pygame.time.Clock()

IMAGE_SURF = pygame.image.load(IMAGE_NAME)
ERASER_SURF = pygame.image.load("eraser.png")
 
width, height = int(IMAGE_SURF.get_width() * 1.1), IMAGE_SURF.get_height()
WORK_ZONE = pygame.Surface((width, height))
WORK_ZONE.fill((255, 255, 255))
WORK_ZONE.blit(IMAGE_SURF, (0, 0))
PROPORTION = width/height
PIXEL_WIDTH = width / 1.1 / PIXELS_X
PIXEL_HEIGHT = height / PIXELS_Y
PALETTE_ITEM_WIDTH = width * (0.1 / 1.1)
PALETTE_ITEM_HEIGHT = height / len(colors)

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


screen = pygame.display.set_mode((INIT_W, INIT_H), RESIZABLE)
w_new = int(min(INIT_W, INIT_H * PROPORTION))
scale_factor = w_new / width
h_new = int(height * scale_factor)
resized_surf = pygame.transform.scale(WORK_ZONE, (w_new, h_new))

mouse_pressed = False
 
def sendColor(x, y, current_color):
    while ser.inWaiting() > 0:
        ser.readall()

    Cmd: str = 'SetPix %d %d, %d %d %d' % (x, y, colors[current_color][0], colors[current_color][1], colors[current_color][2])
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
            mouse_pressed = True
        if event.type == MOUSEBUTTONUP:
            mouse_pressed = False
            color = get_color(event.pos, scale_factor)
            if len(colors) > color > -1:
                current_color = color
                draw_palette_rect(color)
        #if event.type in (MOUSEBUTTONDOWN, MOUSEBUTTONUP) or (event.type == MOUSEMOTION and mouse_pressed):
        if event.type == MOUSEBUTTONDOWN:
            x, y = get_pixel(event.pos, scale_factor)
            if y < PIXELS_Y and x < PIXELS_X:
                if sendColor(x, y, current_color):
                    pygame.draw.rect(
                        PIXELS_SURF,
                        colors[current_color],
                        (PIXEL_WIDTH * x, PIXEL_HEIGHT * y, PIXEL_WIDTH, PIXEL_HEIGHT)
                    )
    screen.fill((0,0,0))
    screen.blit(resized_surf, (0, 0))
    screen.blit(pygame.transform.scale(PIXELS_SURF, (w_new, h_new)), (0,0))
    screen.blit(pygame.transform.scale(RECT_SURF, (w_new, h_new)), (0,0))
    pygame.display.flip()
    fpsClock.tick(fps)