-- Миграция 004: индекс, чтобы быстрее искать товары по категории

CREATE INDEX idx_products_category_id ON products(category_id);
