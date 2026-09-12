idf_component_register(
    SRCS "main.c"
         "ab731_app.c"
         "ab731_quiz.c"
         "idle_power.c"
         "passport_home.c"
         "passport_nav.c"
         "profile_images.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer nvs_flash
)
