# Алгоритм автоматической вертикальной сшивки изображений компьютерной томографии

## Постановка задачи
Имеется набор из нескольких последовательных по высоте и независимо построенных
томографических реконструкций одного объекта в виде воксельных объемов, возможно различной
высоты, с некоторым перекрытием. Требуется произвести их сшивку, то есть выявить перекрытие
соседних реконструкций, удалить его и объединить все в одну непрерывную реконструкцию.
