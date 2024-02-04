export async function GET() {
    return new Response('Hello, Next.js!', { status: 200 })
}

export async function POST(request) {
    const data = await request.json();
    return new Response(JSON.stringify(data), { status: 200 });
}

