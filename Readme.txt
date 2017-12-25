Клиент для проверки токенов.
Запуск тестов: make test
Структура проекта:
	Файлы клиента:
		protocol.h
		protocol.c
		net.h
		net.c
		cli.c
	Юнит-тестирование протокола:
		unittests.c
	Мок-сервер и интеграционный тест:
		fake_server.c
		test.c
