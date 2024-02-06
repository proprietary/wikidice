FROM python:3.12-bookworm AS builder

WORKDIR /usr/src/app

RUN python3 -m venv /venv
ENV PATH="/venv/bin:$PATH"

RUN pip install --upgrade pip

COPY requirements.txt .

RUN pip install --no-cache-dir -r requirements.txt


FROM python:3.12-bookworm AS service
WORKDIR /root/app/site-packages
COPY --from=builder /venv /venv
COPY build build
COPY openapi_server openapi_server
ENV PATH=/venv/bin:$PATH

CMD ["uvicorn", "openapi_server.main:app", "--host", "0.0.0.0", "--port", "8080"]