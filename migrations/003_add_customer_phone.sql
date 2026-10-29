-- Миграция 003: у покупателя появился необязательный телефон

ALTER TABLE customers ADD COLUMN phone VARCHAR(20);
